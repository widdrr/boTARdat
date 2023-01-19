#include "structure.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>

node* new_node(){
    
    node* new_node = malloc(sizeof(node));

    new_node->parent = NULL;
    new_node->children = NULL;
    new_node->path = NULL;
    new_node->name = NULL;
    new_node->entry = archive_entry_new();
    new_node->tempf_name = NULL;
    memset(&new_node->hh,0,sizeof(UT_hash_handle));
    new_node->written = 0;

    return new_node;
}

void free_node(node* die_node){
    
    //since name should always be a substring of path, we only need to free path
    free(die_node->path);
    archive_entry_free(die_node->entry);
    if(die_node->tempf_name != NULL){
        unlink(die_node->tempf_name);
    }
    free(die_node->tempf_name);

    free(die_node);
}

void add_child(node* parent, node* child){
    
    //if node already exists, then it's a placeholder node created during init
    //this is because all other cases check for node existance before adding a new node.
    node* found;
    HASH_FIND_STR(parent->children,child->name,found);
    
    //all we do is replace it's entry
    if(found != NULL){
        archive_entry_free(found->entry);
        found->entry = archive_entry_clone(child->entry);

        free_node(child);
    }
    else {
        child->parent = parent;
        HASH_ADD_STR(parent->children,name,child);

        //if child is a directory, increase the link count of parent, because of '..'
        if(archive_entry_filetype(child->entry) == AE_IFDIR){
            archive_entry_set_nlink(parent->entry,archive_entry_nlink(parent->entry) + 1);
        }
    }
}

int add_path(node* root, node* child){

    //get path for parent 
    char* parent_path = strndup(child->path, child->name - child->path - 1);
    //it excludes the last /, which in the case of the root we want to keep
    if(parent_path[0] == '\0'){
        free(parent_path);
        parent_path = strdup("/");   
    }
    //search parent node
    node* parent = find_node(root,parent_path);

    //if parent doesn't exist, create it temporarely
    //this is because entries in a .TAR file might not be in a DFS-like order
    //however, we are certain to read it's entry later
    if(parent == NULL){
        parent = new_node();

        parent->path = strdup(parent_path);
        parent->name = strrchr(parent->path,'/') + 1;
        
        //recursively try and add the parent
        add_path(root,parent);

    }
    free(parent_path);
    //add the parent-child relationship
    add_child(parent,child);

    return 0;
}

void remove_child(node* child){
    
    if(child->parent == NULL)
        return;
    HASH_DEL(child->parent->children,child);
    child->parent = NULL;
}

int find_entry(archive_entry* entry, int container_fd, archive* out_arc){

    const char* realpath = archive_entry_pathname(entry);
    archive_entry* ent;

    archive_read_support_format_tar(out_arc);

    if(archive_read_open_fd(out_arc,container_fd,BLOCK_SIZE) != ARCHIVE_OK){
        return -archive_errno(out_arc);
    }
    //we search the archive sequentially until we find a matching entry
    //there seems to be no convenient way to get around this
    while(archive_read_next_header(out_arc, &ent)==ARCHIVE_OK){
        const char* path;
        path = archive_entry_pathname(ent);
        
        //we have found our header, calling
        //archive_read_data now returns the desired data
        if(strcmp(realpath,path) == 0){            
            return 0;
        }
        archive_read_data_skip(out_arc);
    }
    //if we didn't find it, clean up
    close_archive(container_fd, out_arc);
    return -ENOENT;
}

void close_archive(int container_fd, archive* out_archive){

    archive_read_free(out_archive);
    lseek(container_fd, 0, SEEK_SET); 
}

void move_to_disk(node* mv_node, int container_fd){

    //if temporary file already exists do nothing
    if(mv_node->tempf_name != NULL){
        return;
    }
    //we initialize the name with the template
    mv_node->tempf_name = strdup(FILE_TEMPLATE);
    //we generate a unique file name, create the file, and open it.
    //the filename is modified in the process
    int temp_fd = mkstemp(mv_node->tempf_name);
    if(container_fd == -1){
        close(temp_fd);
        return;
    }

    archive* read_arc = archive_read_new();
    find_entry(mv_node->entry, container_fd, read_arc);

    //we copy the original contents to our new file.
    char* copy_buffer = malloc(BUFFER_SIZE);
    int read_size;
    while((read_size = archive_read_data(read_arc,copy_buffer,BUFFER_SIZE)) > 0){
        write(temp_fd,copy_buffer,read_size);
    }
    //clean up and reset pointer
    close_archive(container_fd,read_arc);
    //clean memory
    free(copy_buffer);
    close(temp_fd);
}

node* find_node(node* start, const char* path){

    //reaching the end of the path means we found our node
    if(strcmp(path,"/") == 0)
        return start;

    //otherwise, extract everything between the first and second '/'
    char* name_end = strchr(path + 1,'/');
    char* name;

    //handling the case when there is no second /, such as /my_file
    if(name_end == NULL){
        name = strdup(path + 1);
        name_end = strdup("/");
    }
    else{
        name = strndup(path + 1, name_end - path - 1);
    }

    //searching for the node in the hash table
    node* found;
    HASH_FIND_STR(start->children,name,found);

    //desired file or an ancestor was not found
    if(found == NULL){
        free(name);
        if(strcmp(name_end,"/") == 0)
            free(name_end);
        
        return found;
    }
    
    //if ancestor was found, continue searching
    //found might even be the desired file,
    //it that case it will 'search' for '/' within itself, returning itself 
    found = find_node(found,name_end);
    
    //cleaning up
    free(name);
    if(strcmp(name_end,"/") == 0)
        free(name_end);
    
    return found;
}

node* find_by_entry(node* start, archive_entry* entry){
    
    //transform the real_path, see build_tree for more details
    const char* real_path = archive_entry_pathname(entry);
    size_t size = strlen(real_path);

    if(archive_entry_filetype(entry) == AE_IFDIR){
        --size;
    }
    
    char* path = calloc(size + 2, sizeof(char));
    path[0] = '/';
    strncpy(path + 1, real_path, size);

    //find node for real path
    node* found;
    found = find_node(start, path);
    free(path);
    return found;
}

node* find_unwritten(node* start){

    if(start->written == 0)
        return start;
    
    for(node* child = start->children; child!=NULL; child=child->hh.next){
        
        node* found = find_unwritten(child);
        if(found != NULL)
            return found;
    }
    return NULL;

}

void rename_children(node* start){
    
    for(node* child = start->children; child != NULL; child = child->hh.next){
        
        //create new path
        char* parent_path = child->parent->path;
        char* new_path = malloc(strlen(parent_path) + strlen(child->name) + 2);

        //add the parent path and append the file name
        strncpy(new_path,parent_path,strlen(parent_path));
        new_path[strlen(parent_path)]='/';
        strncpy(new_path + strlen(parent_path) + 1, child->name, strlen(child->name) + 1);

        free(child->path);

        child->path = new_path;
        child->name = strrchr(new_path,'/') + 1;
        archive_entry_set_pathname(child->entry,new_path + 1);

        //if directory, don't forget to append '/' to the entry and recurse into it
        if(archive_entry_filetype(child->entry) == AE_IFDIR){
            
            char* entry_path = malloc(strlen(new_path) + 1);
            strncpy(entry_path, new_path + 1, strlen(new_path) - 1);
            entry_path[strlen(new_path) - 1] = '/';
            entry_path[strlen(new_path)] = '\0';

            archive_entry_set_pathname(child->entry,entry_path);

            free(entry_path);

            rename_children(child);
        }

    }
}

int save(node* root, int old_fd, char* name){

    //renames original archive to <name>.tar.old
    char* old_name = malloc(strlen(name) + 5);
    strncpy(old_name, name, strlen(name));
    strcat(old_name,".old");
    rename(name,old_name);
    free(old_name);

    //TO DO... handle exceptions here
    //creates a new archive with the original name
    int new_fd = open(name, O_CREAT | O_WRONLY, S_IRWXU);

    //opens old archive
    archive* old_container = archive_read_new();
    archive_read_support_format_tar(old_container);
    archive_read_open_fd(old_container,old_fd,BLOCK_SIZE);

    //opens new archive
    archive* new_container = archive_write_new();
    archive_write_set_format_pax_restricted(new_container);
    archive_write_open_fd(new_container,new_fd);

    int read_size = -1;
    void* buffer = malloc(BUFFER_SIZE);
    //copies data found in old archive to new archive
    archive_entry* entry;
    while(archive_read_next_header(old_container, &entry) == ARCHIVE_OK){
        
        //for each entry, we search for the node
        node* found = find_by_entry(root,entry);
        if(found == NULL)
            continue;

        //write header
        archive_write_header(new_container,found->entry);
        
        //if the content has since been modified, read from the temp file
        if(found->tempf_name != NULL){
        
            int fd = open(found->tempf_name, O_RDONLY);
            if(fd == -1)
                return -errno;
        
            while((read_size = read(fd,buffer, BUFFER_SIZE)) > 0)
                archive_write_data(new_container, buffer, read_size);
        }
        //otherwise if it's not a directory, copy from old to new
        else if(archive_entry_filetype(found->entry) == AE_IFREG){
            
            while((read_size = archive_read_data(old_container, buffer, BUFFER_SIZE)) > 0){
                archive_write_data(new_container, buffer, read_size);
            }
        }
        //mark node as written
        found->written = 1;
    }
    free(buffer);
    
    if(save_new(root,new_container) != 0)
        return -errno;

    //close archives and clean up
    close_archive(old_fd,old_container);
    close_archive(new_fd,new_container);
    close(new_fd);
    return 0;
}

int save_new(node* start, archive* new_arch){
    
    if(start->written == 0){
        
        archive_write_header(new_arch,start->entry);
        if(start->tempf_name != NULL){
            
            int fd = open(start->tempf_name, O_RDONLY);
            if(fd == -1)
                return -errno;
            
            int read_size = -1;
            void* buffer = malloc(BUFFER_SIZE);
            
            while((read_size = read(fd,buffer, BUFFER_SIZE)) > 0)
                archive_write_data(new_arch, buffer, read_size);
            
            free(buffer);
        }
        start->written = 1;
    }
    for(node* child = start->children; child != NULL; child = child->hh.next){
        if(save_new(child,new_arch) != 0)
            return -errno;
    }
    return 0;
}

int build_tree(node* root, int archive_fd, struct stat* mount_st){

    struct archive *container = archive_read_new();
    archive_read_support_format_tar(container);
    //opens the archive for reading
    //todo: handle errors

    if(archive_read_open_fd(container,archive_fd,BLOCK_SIZE) != ARCHIVE_OK){
        return -archive_errno(container);
    }

    //populating root
    //root had to have been initialized 
    root->path = strdup("/");
    root->name = root->path;

    struct stat st;
    fstat(archive_fd, &st);

    //create archive_entry for root from archive stats
    archive_entry_set_uid(root->entry,st.st_uid);
    archive_entry_set_gid(root->entry,st.st_gid);
    archive_entry_set_mtime(root->entry,st.st_mtime,0);
    archive_entry_set_atime(root->entry,st.st_atime,0);
    archive_entry_set_ctime(root->entry,st.st_ctime,0);
    archive_entry_set_size(root->entry,0);
    archive_entry_set_nlink(root->entry,2);

    //only set mode from mount directory
    archive_entry_set_mode(root->entry,mount_st->st_mode);

    node* new_file = new_node();
    //as long as we can read archive headers, create and add nodes
    while(archive_read_next_header2(container,new_file->entry) == ARCHIVE_OK){

        //populate path and name
        const char* path = archive_entry_pathname(new_file->entry);
        size_t size = strlen(path);

        //all files have 1 hardlink by default, their name.
        archive_entry_set_nlink(new_file->entry,1);

        archive_entry_set_atime(root->entry,time(NULL),0);
        archive_entry_set_ctime(root->entry,time(NULL),0);

        //if entry is a directory, it ends with '/' and we need to remove it
        //we also set the hardlink count to 2 while we're at it, since directories have '.'
        if(archive_entry_filetype(new_file->entry) == AE_IFDIR){
            --size;
            archive_entry_set_nlink(new_file->entry,2); 
        }
    
        //copy file path and prepend '/'
        new_file->path = calloc(size + 2, sizeof(char));
        new_file->path[0] = '/';
        strncpy(new_file->path + 1, path,size);

        new_file->name = strrchr(new_file->path,'/') + 1;

        if(add_path(root,new_file) != 0){
            return -errno;
        }
        new_file = new_node();
    }
    //free the extra node
    free_node(new_file);
    //free archive and reset pointer
    close_archive(archive_fd, container);
    return 0;
}

void burn_tree(node* start){

    //if node is a directory, recurse into every child first
    if(archive_entry_filetype(start->entry) == AE_IFDIR){
        //this approach is memory safe compared to the previous for loop
        node* child = start->children;
        while(child != NULL){
            node* next_child = child->hh.next;
            burn_tree(child);
            child = next_child;
        }
    }
    //can safely delete node now
    remove_child(start);
    free_node(start);
}