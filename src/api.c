#include "api.h"
#include "structure.h"

void* btrdt_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    //todo, look into these parameters and decide if they are useful
    (void)conn;
    (void)cfg;

    printf("Init called\n");
    //fuse_get_context() returns a struct with info about the curent context
    //in the struct we can find the private data we passed to fuse_main;
    btrdt_data* fs_data = fuse_get_context()->private_data;

    //we build the directory structure
    build_tree(fs_data->root,
              fs_data->container,
              fs_data->archive_name,
              fs_data->mount_point);
    
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
    free(fs_data->mount_point);
    free(fs_data->archive_name);

    //free the data struct
    free(fs_data);
}