#pragma once

#include <archive.h>
#include <archive_entry.h>
#include "uthash.h"

//libarchive determines the block_size automatically
//this is the fallback value    
#define BLOCK_SIZE 10240
#define BUFFER_SIZE 10240
#define FILE_TEMPLATE P_tmpdir"/botardat-XXXXXX"

typedef struct archive archive;
typedef struct archive_entry archive_entry;

typedef struct node{

    struct node* parent; //node parent
    struct node* children; // hash of all children, key is name
    
    char* path; //full path of node
    char* name; //basename of node

    archive_entry* entry; //archive header data
    char* tempf_name; //path of temporary file for writing

    UT_hash_handle hh; // this makes node hashable
    int written; // keeps track of whether we wrote this one or not
    
} node;

//creates and NULL initializes a new node 
node* new_node();

//deallocates memory for a node
//node should be completely isolated so call remove_child before
void free_node(node* node);

//add child node to parent node
void add_child(node* parent, node* child);

//adds child node by path
int add_path(node* root,node* child);

//removes child from node parent
void remove_child(node* child);

//finds the header in the archive for the given entry
//returns the out_archive parameter at the begining of the data
//you can then call archive_read_data() on it.
//should always call close_archive() once you're done with it,
//unless this returns an error;
int find_entry(archive_entry* entry, int container_fd, archive* out_archive);

//frees the archive struct and resets the file pointer to the begining
void close_archive(int container_fd, archive* out_archive);

//creates a temporaryfile on disk and moves all contents
//if only creation is desired, pass -1 to fd
void move_to_disk(node* mv_node, int container_fd);

//returns node for given path starting from given root
node* find_node(node* start, const char* path);

//returns the node for the given entry if it exists
node* find_by_entry(node* start, archive_entry* entry);

//returns the first node found that has not been written
node* find_unwritten(node* start);

//fixes the path of every descendant for a renamed directory
void rename_children(node* node);

//saves the contents of the directory structure to a new archive
int save(node* root, int old_fd, char* name);

//adds all new files to a new archive
int save_new(node* start, archive* new_arch);

//builds directory structure tree from archive with given file descriptor
//struct stat for mount point is used to set the root's mode
int build_tree(node* root, int archive_fd, struct stat* mount_st);

//deconstructs the directory structure tree and frees all memory
void burn_tree(node* start);
