#include "api.h"
#include "structure.h"
#include <errno.h>


//fuse_operations struct has the mapping between
//fuse opererations and our implementations
static const struct fuse_operations btrdt_op ={
    .init = btrdt_init,
    .destroy = btrdt_destroy,
    .getattr = btrdt_getattr,
    .readdir = btrdt_readdir,
    .read = btrdt_read,
    .mknod = btrdt_mknod,
    .utimens = btrdt_utimens,
    .chmod = btrdt_chmod,
    .chown = btrdt_chown,
    .write = btrdt_write,
    .mkdir = btrdt_mkdir,
    .unlink = btrdt_unlink,
    .rmdir = btrdt_rmdir,
    .truncate = btrdt_truncate,
    .rename = btrdt_rename,
    .open = btrdt_open,
    .release = btrdt_release,
};

//argument parsing function
//for a given argument, returning 0 = argument does not get passed to FUSE
// returning 1 = argument gets passed to fuse
static int btrdt_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {

    btrdt_data* fs_data = data;

    //argument does not start with - (it's not an option)
    if (key == FUSE_OPT_KEY_NONOPT) {

        //first argument should be archive name, so set it
        //and get it's file descriptor
        if(fs_data->archive_name == NULL){
            
            fs_data->archive_name = strdup(arg);
            fs_data->archive_fd = open(arg,O_RDONLY);
            return 0;
        }
        //second argument should be the mount directory
        //we save it's struct stat and pass it to FUSE
        if(fs_data->mount_name == NULL){
            
            fs_data->mount_name = strdup(arg);
            stat(arg,&fs_data->mount_st);
            return 1;
        }
    }
    //any other flag gets passed to FUSE
    return 1;
}


int main(int argc, char* argv[]){

    //initializing fuse
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    //initializing our metadata struct
    btrdt_data* fs_data = malloc(sizeof(btrdt_data));
    fs_data->archive_name = NULL;
    fs_data->mount_name = NULL;
    fs_data->working_dir = getcwd(NULL,0);

    //fuse_opt_parse automatically parses the args using a configuration struct and a parsing function
    fuse_opt_parse(&args,fs_data,NULL,btrdt_opt_proc);
    
    //for now we force FUSE to run in single thread mode, multithread support to be determined
    fuse_opt_add_arg(&args,"-s");   
    if(fuse_main(args.argc,args.argv,&btrdt_op,fs_data)){
        return errno;
    }

    fuse_opt_free_args(&args);
    return 0;
}