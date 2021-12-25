#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <stdbool.h>
#include <string.h>
#include <algorithm> //used for std::sort
#include "file.h"
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// FUNCTION PROTOTYPES.

// File Index Manager APIs.
int init_db();
int shutdown_db();
int64_t open_table(char* pathname);
int db_insert( int64_t table_id, int64_t key, char * value, uint16_t val_size );
int db_find(int64_t table_id, int64_t key, char * ret_val, uint16_t * val_size);
int db_delete(int64_t table_id, int64_t key);
// FIND.

pagenum_t find_leaf( int64_t table_id, pagenum_t root, int64_t key );
slot_t * find( int64_t table_id, pagenum_t root, int64_t key );

// Insertion.

slot_t * make_slot(int64_t key, uint16_t val_size);
pagenum_t make_node( int64_t table_id );
pagenum_t make_leaf( int64_t table_id );
void insert_into_leaf( int64_t table_id, pagenum_t leafnum, slot_t * pointer, char * value );
void insert_into_leaf_after_splitting(int64_t table_id, pagenum_t root, pagenum_t leafnum, slot_t pointer, char * value);
void insert_into_node(int64_t table_id, pagenum_t  parent,
        int left_index, int64_t key, pagenum_t rightnum);
void insert_into_node_after_splitting(int64_t table_id, pagenum_t old_nodenum, int left_index, int64_t key, pagenum_t rightnum);
void insert_into_parent(int64_t table_id, pagenum_t leftnum, int64_t key, pagenum_t rightnum);
void insert_into_new_root(int64_t table_id, pagenum_t leftnum, int64_t key, pagenum_t rightnum);
void start_new_tree(int64_t table_id, slot_t * pointer, char * value);

// Deletion.

int get_neighbor_index( int64_t table_id, pagenum_t n );
bool cmp_slot(const slot_t & b1, const slot_t & b2);
void remove_entry_from_node(int64_t table_id, pagenum_t n, int64_t key);
void adjust_root(int64_t table_id);
void coalesce_nodes(int64_t table_id, pagenum_t n, pagenum_t neighbor,
                      int neighbor_index, int64_t k_prime);
void redistribute_nodes(int64_t table_id, pagenum_t n, pagenum_t neighbor, int neighbor_index,int k_prime_index, int64_t k_prime);
void delete_entry( int64_t table_id, pagenum_t n, int64_t key );


#endif /* __BPT_H__*/
