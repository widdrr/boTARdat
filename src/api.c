#include "api.h"
#include "structure.h"
#include <errno.h>

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
    //deconstruct our structure
    burn_tree(fs_data->root);

    //free archive
    archive_free(fs_data->container);

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

    filler(buffer, ".", NULL, 0,0);
	filler(buffer, "..", NULL, 0,0);

    for(node* child = dir->children; child!=NULL; child=child->hh.next){
        filler(buffer,child->name,NULL,0,0);
    }
    
    return 0;
}
int btrdt_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi ){
    btrdt_data* fs_data = fuse_get_context()->private_data;
    node* found = find_node(fs_data->root,path);
    int read_size = -ENOENT;
    const char* realpath;
    struct archive_entry *entry;

    if(found == NULL){
        return -ENOENT;
    }
    realpath = archive_entry_pathname(found->entry);
    struct archive *arc = archive_read_new();

    if(archive_read_open_fd(arc,fs_data->archive_fd,BLOCK_SIZE) != ARCHIVE_OK)
        return -archive_errno(arc);
    
    while(archive_read_next_header(arc,&entry)==ARCHIVE_OK){
        const char* path;
        path = archive_entry_pathname(entry);
        if(strcmp(realpath,path) == 0){
            if(offset + size > archive_entry_size(entry)){
                return 0;
            }
            void* trash = malloc(IO_BLOCK);
            while(offset){
                int skip;
                if(offset > IO_BLOCK)
                    skip = offset;
                else
                    skip = IO_BLOCK;
                archive_read_data(arc,trash,skip);
                //warnings
                offset = offset - skip;
            }   
            free(trash);
            read_size = archive_read_data(arc,buffer,size);
            //warnings
            break;
        }
        archive_read_data_skip(arc);
    }
    archive_read_free(arc);
    lseek(fs_data->archive_fd, 0, SEEK_SET);
    return read_size;
}
