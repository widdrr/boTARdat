#include "structure.h"


int main(int argc, char* argv[]){

    node* root = NULL;
    archive* arch = NULL;

    int err = build_tree(&root,&arch,argv[1],argv[2]);
    printf("Err: %d\n",err);
    printf("%d\n",archive_entry_filetype(root->children->entry));
    burn_tree(root);
}