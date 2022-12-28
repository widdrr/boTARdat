#pragma once

#include <archive.h>
#include <archive_entry.h>
#include "uthash.h"

//libarchive determines the block_size automatically
//this is the fallback value    
#define BLOCK_SIZE 10240
#define IO_BLOCK 4096

typedef struct archive archive;
typedef struct archive_entry archive_entry;

typedef struct node{

    struct node* parent; //node parent
    struct node* children; // hash of all children, key is name
    
    char* path; //full path of node
    char* name; //basename of node

    archive_entry* entry; //archive header data
    char* temp; //location of temporary file for writing

    UT_hash_handle hh; // this makes node hashable

} node;

//creates and NULL initializes a new node 
node* new_node();

//deallocates memory for a node
//node should be completely isolated so call remove_child before
void free_node(node* node);

//add child node to parent node
void add_child(node* parent, node* child);

//removes child from node parent
void remove_child(node* child);

//returns node for given path starting from given root
node* find_node(node* root, const char* path);

//builds directory structure tree from archive with given file descriptor
//struct stat for mount point is used to set the root's mode
int build_tree(node* root, int archive_fd, struct stat* mount_st);

//deconstructs the directory structure tree and frees all memory
void burn_tree(node* start);
