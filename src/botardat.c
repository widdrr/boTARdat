#include "api.h"
#include "structure.h"
#include <errno.h>


static const struct fuse_operations btrdt_op ={
    .init = btrdt_init,
    .destroy = btrdt_destroy,
    
};

static int btrdt_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {

    btrdt_data* fs_data = data;

     if (key == FUSE_OPT_KEY_NONOPT) {
        if(fs_data->archive_name == NULL){
            fs_data->archive_name = strdup(arg);
            fs_data->archive_fd = open(arg,O_RDONLY);
            return 0;
        }
        if(fs_data->mount_name == NULL){
            fs_data->mount_name = strdup(arg);
            stat(arg,&fs_data->mount_st);
        }
        return 1;
    }
    return 1;
}


int main(int argc, char* argv[]){

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    btrdt_data* fs_data = malloc(sizeof(btrdt_data));

    fs_data->working_dir = getcwd(NULL,0);

    fuse_opt_parse(&args,fs_data,NULL,btrdt_opt_proc);
    if(fuse_main(args.argc,args.argv,&btrdt_op,fs_data)){
        return errno;
    }
    return 0;
}