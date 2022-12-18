#include "structure.h"
#include <stdio.h>

node* new_node(){
    
    node* new_node = malloc(sizeof(node));

    new_node->parent = NULL;
    new_node->children = NULL;
    new_node->path = NULL;
    new_node->name = NULL;
    new_node->entry = archive_entry_new();
    new_node->temp = NULL;
    memset(&new_node->hh,0,sizeof(UT_hash_handle));
}

void free_node(node* die_node){
    
    free(die_node->path);
    free(die_node->name);
    archive_entry_free(die_node->entry);
    free(die_node->temp);

}