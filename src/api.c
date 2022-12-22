#include "api.h"
#include "structure.h"

void* btrdt_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    //todo, look into these parameters and decide if they are useful
    (void)conn;
    (void)cfg;

    //fuse_get_context() returns a struct with into about the curent context
    //in the struct we can find the private data we passed to fuse_main;
    btrdt_context* fs_data = fuse_get_context()->private_data;

    //we build the directory structure
    build_tree(fs_data->root,
              fs_data->container,
              fs_data->archive_name,
              fs_data->mount_point);
    
    //we return the now modified filesystem metadata
    return fs_data;
}