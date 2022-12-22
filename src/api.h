#pragma once

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>
#include <fuse3/fuse_common.h>
#include "structure.h"

typedef struct btrdt_context {

    node* root;
    archive* container;
    char* archive_name;
    char* mount_point;

} btrdt_context;

//operation that gets called upon filesystem initialization
//it returns a btrdt_context struct which holds all our filesystem metadata. 
void* btrdt_init(struct fuse_conn_info *conn, struct fuse_config *cfg);

//operation that gets called upon unmount
//we will clean up our filesystem.
void btrdt_destroy(void* fs_data);


