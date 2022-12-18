#include <archive_entry.h>
#include "uthash.h"

typedef struct node{

    struct node* parent; //node parent
    struct node* children; // hash of all children, key is name
    
    char* path; //full path of node
    char* name; //basename of node

    struct archive_entry* entry; //archive header data
    char* temp; //location of temporary file for writing

    UT_hash_handle hh; // this makes node hashable

} node;

//creates and NULL initializes a new node 
node* new_node();

//deallocates memory for a node
//node should not have any children
void free_node(node* node);

