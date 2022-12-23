#include "structure.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>

node* new_node(){
    
    node* new_node = malloc(sizeof(node));

    new_node->parent = NULL;
    new_node->children = NULL;
    new_node->path = NULL;
    new_node->name = NULL;
    new_node->entry = archive_entry_new();
    new_node->temp = NULL;
    memset(&new_node->hh,0,sizeof(UT_hash_handle));

    return new_node;
}

void free_node(node* die_node){
    
    free(die_node->path);
    free(die_node->name);
    archive_entry_free(die_node->entry);
    free(die_node->temp);

    free(die_node);
}

void add_child(node* parent, node* child){
    
    //todo check if child already exists
    child->parent = parent;
    HASH_ADD_STR(parent->children,name,child);
    //if child is a directory, increase the link count of parent, because of '..'
    if(archive_entry_filetype(child->entry) == AE_IFDIR){
        if(strcmp(parent->name,"/") == 0){
        printf("Added child to root, it has %d links\n",archive_entry_nlink(parent->entry));
        }   
        archive_entry_set_nlink(parent->entry,archive_entry_nlink(parent->entry) + 1);
    }
}

void remove_child(node* rem_node){
    
    if(rem_node->parent == NULL)
        return;
    HASH_DEL(rem_node->parent->children,rem_node);
    rem_node->parent = NULL;
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

    if(found == NULL)
        return found;
    found = find_node(found,name_end);
    free(name);
    if(strcmp(name_end,"/") == 0)
        free(name_end);
    return found;
}

int build_tree(node* root, archive* container, int archive_fd, struct stat* mount_st){

    //opens the archive for reading
    //todo: handle errors
    if(archive_read_open_fd(container,archive_fd,BLOCK_SIZE) != ARCHIVE_OK){
        return archive_errno(container);
    }
    //populating root
    //root had to have been initialized 

    root->path = strdup("/");
    root->name = strdup("/");

    struct stat st;
    fstat(archive_fd, &st);

    //create archive_entry for root from archive stats
    archive_entry_set_uid(root->entry,st.st_uid);
    archive_entry_set_gid(root->entry,st.st_gid);
    archive_entry_set_mtime(root->entry,st.st_mtime,0);
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

        char* slash_pos = strrchr(new_file->path,'/');
        new_file->name = strdup(slash_pos + 1);

        //get path for parent 
        char* parent_path = strndup(new_file->path, slash_pos - new_file->path);
        //it excludes the last /, which in the case of the root we want to keep
        if(parent_path[0] == '\0'){
            free(parent_path);
            parent_path = strdup("/");   
        }
        //search parent node
        node* parent = find_node(root,parent_path);
        free(parent_path);

        if(parent == NULL){
            return -ENOENT;
        }
        //add the parent-child relationship
        add_child(parent,new_file);
        new_file = new_node();
    }
    //free the extra node
    free_node(new_file);
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