#include "structure.h"


int main(int argc, char* argv[]){

    if(!build_tree(argv[1],argv[2])){
        return -1;
    }
    burn_tree(root);
    return 0;
}