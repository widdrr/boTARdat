#pragma once

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <fuse3/fuse_common.h>
#include "structure.h"

//this struct contains metadata for the directory structure
typedef struct btrdt_data {

    node* root; //root node
    char* archive_name; // name of opened archive
    int archive_fd; //file descriptor for opened archive
    char* mount_name; //name of mount directory
    struct stat mount_st; //struct stat for mount directory (once FUSE gets mounted it appears to be lost until unmount)
    char* working_dir; //path for the working directory (might prove useful)

} btrdt_data;

//operation that gets called upon filesystem initialization
//it returns a btrdt_data struct which holds all our filesystem metadata. 
void* btrdt_init(struct fuse_conn_info* conn, struct fuse_config* cfg);

//operation that gets called upon unmount
//we will clean up our filesystem.
void btrdt_destroy(void* private_data);

//operation that returns a struct stat for given path
int btrdt_getattr(const char* path, struct stat* st, struct fuse_file_info* fi);

//operation that returns the names of all contents of a directory in a buffer
int btrdt_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags);

//operation that reads the request bytes from given offset into the buffer
int btrdt_read(const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi);

//operation that creates new file for give path and mode
int btrdt_mknod(const char *path, mode_t mode, dev_t dev);

//opertion that changes access and modified times for gived fiel
int btrdt_utimens(const char *path, const struct timespec times[2], struct fuse_file_info *fi);

