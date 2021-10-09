#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "file.h"
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

#define DEFAULT_ORDER 4

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */
typedef struct record {
    int value;
} record;

/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */
typedef struct node {
    void ** pointers;
    int * keys;
    struct node * parent;
    bool is_leaf;
    int num_keys;
    struct node * next; // Used for queue.
} node;

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int order;


// FUNCTION PROTOTYPES.

int init_db();
int shutdown_db();
int64_t open_table(char* pathname);

// Utility.

int height( node * root );
int path_to_root( node * root, node * child );
//void find_and_print(node * root, int key );
pagenum_t find_leaf( int64_t table_id, pagenum_t root, int key );
slot_t * find( int64_t table_id, pagenum_t root, int key );
int cut( int length );

// Insertion.

slot_t * make_slot(int key, uint16_t val_size);
pagenum_t make_node( int64_t table_id );
pagenum_t make_leaf( int64_t table_id );
//int get_left_index(node * parent, node * left);
void insert_into_leaf( int64_t table_id, pagenum_t leafnum, slot_t * pointer, char * value );
void insert_into_leaf_after_splitting(int64_t table_id, pagenum_t root, pagenum_t leafnum, slot_t pointer, char * value);
void insert_into_node(int64_t table_id, pagenum_t  parent,
        int left_index, uint64_t key, pagenum_t rightnum);
node * insert_into_node_after_splitting(node * root, node * parent,
                                        int left_index,
        int key, node * right);
void insert_into_parent(int64_t table_id, pagenum_t root, pagenum_t leftnum, uint64_t key, pagenum_t rightnum);
void insert_into_new_root(int64_t table_id, pagenum_t leftnum, uint64_t key, pagenum_t rightnum);
void start_new_tree(int64_t table_id, record * pointer,char * value);
int db_insert( int64_t table_id, int64_t key, char * value, uint16_t val_size );

// Deletion.

int get_neighbor_index( node * n );
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
                      int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
                          int neighbor_index,
        int k_prime_index, int k_prime);
node * delete_entry( node * root, node * n, int key, void * pointer );
node * db_delete( node * root, int key );

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);

#endif /* __BPT_H__*/
