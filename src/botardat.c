#include "api.h"
#include "structure.h"
#include <errno.h>


static const struct fuse_operations btrdt_op ={
    
};

// static int myfs_opt_proc(void *data, const char *arg, int key, struct fuse_args *outargs) {
     
//      if (key == FUSE_OPT_KEY_NONOPT) {
//         myparameter = strdup(arg);
//         return 0;
     
// }


int main(int argc, char* argv[]){

    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

    fuse_opt_parse(&args,)
    if(fuse_main(args.argc,args.argv,&btrdt_op,NULL)){
        return errno;
    }

    burn_tree();
    return 0;
}