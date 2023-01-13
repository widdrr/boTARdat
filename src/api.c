#include "api.h"
#include "structure.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

void* btrdt_init(struct fuse_conn_info *conn, struct fuse_config *cfg){

    //todo, look into these parameters and decide if they are useful
    (void)conn;
    (void)cfg;

    //fuse_get_context() returns a struct with info about the curent context
    //in the struct we can find the private data we passed to fuse_main;
    btrdt_data* fs_data = fuse_get_context()->private_data;

    fs_data->root = new_node();

    //we build the directory structure
    build_tree(fs_data->root,
              fs_data->archive_fd,
              &fs_data->mount_st);
    
    //we return the now modified filesystem metadata
    return fs_data;
}

void btrdt_destroy(void* private_data){

    btrdt_data *fs_data = private_data;
    
    save(fs_data->root, fs_data->archive_fd, fs_data->archive_name);

    burn_tree(fs_data->root);


    //free other fields
    free(fs_data->archive_name);
    free(fs_data->mount_name);
    free(fs_data->working_dir);

    //close the fd's
    close(fs_data->archive_fd);
    //free the data struct
    free(fs_data);
}

int btrdt_getattr(const char *path, struct stat* st, struct fuse_file_info* fi){

    btrdt_data* fs_data = fuse_get_context()->private_data;
    
    node* found = find_node(fs_data->root, path);
    if(found == NULL){
        return -ENOENT;
    }

    //extracting all relevant values;
    st->st_gid = archive_entry_gid(found->entry);
    st->st_uid = archive_entry_uid(found->entry);
    st->st_mode = archive_entry_mode(found->entry);
    st->st_size = archive_entry_size(found->entry);
    st->st_nlink = archive_entry_nlink(found->entry);
    st->st_mtim.tv_sec = archive_entry_mtime(found->entry);
    
    return 0;
}

int btrdt_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags){
    
    btrdt_data* fs_data = fuse_get_context()->private_data;

    node* dir = find_node(fs_data->root,path);
    if(dir==NULL){
        return -ENOENT;
    }

    //every directory has . and .. as entries by default
    filler(buffer, ".", NULL, 0,0);
	filler(buffer, "..", NULL, 0,0);

    //add each child
    for(node* child = dir->children; child!=NULL; child=child->hh.next){
        filler(buffer,child->name,NULL,0,0);
    }
    
    return 0;
}
int btrdt_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){
    
    btrdt_data* fs_data = fuse_get_context()->private_data;
    node* found = find_node(fs_data->root,path);

    if(found == NULL){
        return -ENOENT;
    }

    //if reading bytes outside the file, don't
    if(offset > archive_entry_size(found->entry)){
        return 0;
    }

    int read_size = -1;

    //if node has a temp file, read from it
    if(found->tempf_name != NULL){
        
        int fd = open(found->tempf_name, O_RDONLY);
        read_size = pread(fd,buffer,size,offset);
        close(fd);
        
        return read_size; 
    }

    //find the location of the entry in the archive
    struct archive *arc = archive_read_new();
    int err;
    if((err = find_entry(found->entry, fs_data->archive_fd, arc)) != 0){
        return -err;
    }
    //skip the offset by reading and discarding
    //also no convenient way around this.
    void* trash = malloc(BUFFER_SIZE);
    while(offset){
        int skip;
        if(offset > BUFFER_SIZE)
            skip = BUFFER_SIZE;
        else
            skip = offset;
    //todo handle errors
        archive_read_data(arc,trash, skip);
        offset = offset - skip;
    } 
    free(trash);
    //todo handle errors
    read_size = archive_read_data(arc, buffer, size);
    //clean up and reset pointer
    close_archive(fs_data->archive_fd, arc);
    return read_size;
}

int btrdt_mknod(const char *path, mode_t mode, dev_t dev){
    
    btrdt_data* fs_data = fuse_get_context()->private_data;

    //if file exists, error
    if(find_node(fs_data->root,path) != NULL){
        return -EEXIST;
    }

    node* new_file = new_node();

    //we set path and name
    new_file->path = strdup(path);
    new_file->name = strrchr(new_file->path,'/') + 1;

    if(add_path(fs_data->root,new_file) != 0){
        return -errno;
    }

    //set all relevant archive entry stat values
    archive_entry_set_pathname(new_file->entry, new_file->path + 1);
    archive_entry_set_uid(new_file->entry,getuid());
    archive_entry_set_gid(new_file->entry,getgid());
    archive_entry_set_mtime(new_file->entry,time(NULL),0);
    archive_entry_set_mode(new_file->entry,mode);
    archive_entry_set_size(new_file->entry,0);
    archive_entry_set_nlink(new_file->entry,1);
    archive_entry_set_dev(new_file->entry,dev);

    //since this is a new file, we have to create a backing temp on disk for content
    move_to_disk(new_file,-1);

    return 0;
}

int btrdt_utimens(const char *path, const struct timespec times[2], struct fuse_file_info *fi){

    btrdt_data* fs_data = fuse_get_context()->private_data;

    node* found = find_node(fs_data->root,path);
    if(found == NULL){
        return -ENOENT;
    }

    //set last access and last modified times
    archive_entry_set_atime(found->entry,times[0].tv_sec,0);
    archive_entry_set_mtime(found->entry,times[1].tv_sec,0);

    return 0;
}

int btrdt_chmod(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    btrdt_data* fs_data = fuse_get_context()->private_data;

    node* found = find_node(fs_data->root,path);
    if(found == NULL){
        return -ENOENT;
    }

    archive_entry_set_mode(found->entry, mode);

    return 0;
}

int btrdt_chown(const char *path, uid_t owner, gid_t group, struct fuse_file_info *fi)
{
    btrdt_data* fs_data = fuse_get_context()->private_data;

    node* found = find_node(fs_data->root,path);
    if(found == NULL){
        return -ENOENT;
    }

    //-1 represents missing parameter therefore it does not require change
    if(owner != -1)
        archive_entry_set_uid(found->entry, owner);
    if(group != -1)
        archive_entry_set_gid(found->entry, group);

    return 0;
}

int btrdt_write(const char *path, const char *buf, size_t size,off_t offset, struct fuse_file_info *fi)
{
    btrdt_data* fs_data = fuse_get_context()->private_data;
    node* found = find_node(fs_data->root,path);
    if(found == NULL)
    {
        return -ENOENT;
    }
    int fh;
    //create a temp file for the current node, if it already has one then it stops
    move_to_disk(found,fs_data->archive_fd);
    fh = open(found->tempf_name,O_WRONLY);
    pwrite(fh,buf,size,offset);
    close(fh);
    return 0;
}