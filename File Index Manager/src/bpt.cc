/*
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 */

#include "bpt.h"

// GLOBALS.
int order = DEFAULT_ORDER;

// FUNCTION DEFINITIONS.

int init_db(){
    
    return 0;
}

int shutdown_db(){
    file_close_database_file();
    printf("DB shutdowns.\n");
    exit(0);
}

int64_t open_table(char* pathname){
    return file_open_database_file(pathname);
}

// OUTPUT AND UTILITIES


/* Utility function to give the height
 * of the tree, which length in number of edges
 * of the path from the root to any leaf.
 */
int height( node * root ) {
    int h = 0;
    node * c = root;
    while (!c->is_leaf) {
        c = (node *)c->pointers[0];
        h++;
    }
    return h;
}


/* Utility function to give the length in edges
 * of the path from any node to the root.
 */
int path_to_root( node * root, node * child ) {
    int length = 0;
    node * c = child;
    while (c != root) {
        c = c->parent;
        length++;
    }
    return length;
}


/* Finds the record under a given key and prints an
 * appropriate message to stdout.
 */
/*void find_and_print(node * root, int key ) {
    record * r = find(root, key);
    if (r == NULL)
        printf("Record not found under key %d.\n", key);
    else 
        printf("Record at %lx -- key %d, value %d.\n",
                (unsigned long)r, key, r->value);
}
*/

/* Traces the path from the root to a leaf, searching
 * by key. Returns the leaf containing the given key.
 */
pagenum_t find_leaf( int64_t table_id, pagenum_t root, int key ) {
    int i = 0;
    page_t * c = new page_t;
    if (c == NULL) {
        perror("find leaf error");
        exit(EXIT_FAILURE);
    }
    file_read_page(table_id,root,c);  //read root page
    
    pagenum_t next;
    while (!c->IsLeaf) {
        i = 0;
        while (i < c->keys) {
            branch_t temp;
            memcpy(&temp,c->free_space+i*16,sizeof(branch_t));
            if (key >= temp.key ) i++;
            else break;
        }
        
        if (i == 0) memcpy(&next,&c->next,sizeof(pagenum_t));
        else memcpy(&next,c->free_space+i*16-8,sizeof(pagenum_t));
        file_read_page(table_id,next,c);
    }
    
    delete c;
    return next;
}


/* Finds and returns the record to which
 * a key refers.
 */
slot_t * find( int64_t table_id, pagenum_t root, int key ) {
    int i = 0;
    page_t * c =  new page_t;
    if(c == NULL){
        perror("find error");
        exit(EXIT_FAILURE);
    }
    file_read_page(table_id,find_leaf( table_id, root, key ),c);
    
    if (c == NULL) return NULL;
    
    slot_t * temp = new slot_t;
    for (i = 0; i < c->keys; i++){
        memcpy(temp,c->free_space+i*12,12); //sizeof(slot_t) == 12
        if (temp->key == key){
            delete c;
            return temp;
        }
        
    }
    delete c;
    delete temp;
    return NULL;
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


// INSERTION

/* Creates a new record to hold the value
 * to which a key refers.
 */
slot_t * make_slot(int key, uint16_t val_size) {
    slot_t * new_slot = new slot_t;
    if (new_slot == NULL) {
        perror("Slot creation error");
        exit(EXIT_FAILURE);
    }
    else {
        new_slot->key = key;
        new_slot->val_size = val_size;
        new_slot->offset = 0;
        //offset will be set when inserted
    }
    return new_slot;
}


/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
pagenum_t make_node( int64_t table_id ) {
    page_t new_page;
    pagenum_t new_pagenum = file_alloc_page(table_id);
    file_read_page(table_id,new_pagenum,&new_page);
    
    new_page.parent = 0;
    new_page.IsLeaf = 0;  //default - internal
    new_page.keys = 0;
    new_page.free_amount = 0;
    new_page.next = 0;
    
    file_write_page(table_id,new_pagenum,&new_page);
    return new_pagenum;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
pagenum_t make_leaf( int64_t table_id ) {
    pagenum_t leafnum = make_node(table_id);
    page_t leaf;
    file_read_page(table_id,leafnum,&leaf);
    leaf.IsLeaf = 1;
    leaf.free_amount = 3968;
    file_write_page(table_id,leafnum,&leaf);
    return leafnum;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pointer to 
 * the node to the left of the key to be inserted.
 
int get_left_index(node * parent, node * left) {

    int left_index = 0;
    while (left_index <= parent->num_keys && 
            parent->pointers[left_index] != left)
        left_index++;
    return left_index;
}
 */

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
void insert_into_leaf( int64_t table_id, pagenum_t leafnum, slot_t * pointer, char * value ) {
    
    page_t leaf;
    file_read_page(table_id,leafnum,&leaf);
    
    int i, insertion_point;

    insertion_point = 0;
    while (insertion_point < leaf.keys){
        slot_t temp;
        memcpy(&temp,leaf.free_space+insertion_point
               *12,12);
        if(temp.key<pointer->key) insertion_point++;
        else break;
    }

    //sort keys
    for (i = leaf.keys; i > insertion_point; i--) {
        memcpy(leaf.free_space+i*12,leaf.free_space+(i-1)*12,12);
    }
    //set offset
    pointer->offset = 12*leaf.keys+leaf.free_amount-pointer->val_size;
    memcpy(leaf.free_space+insertion_point*12,pointer,12);
    leaf.keys++;
    memcpy(leaf.free_space+pointer->offset,value,pointer->val_size);
    leaf.free_amount-=(12+pointer->val_size);
    file_write_page(table_id,leafnum,&leaf);
    
    delete pointer;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
void insert_into_leaf_after_splitting(int64_t table_id, pagenum_t root, pagenum_t leafnum, slot_t * pointer, char * value) {

    pagenum_t new_leafnum = make_leaf(table_id);
    page_t leaf;
    page_t new_leaf;
    file_read_page(table_id,leafnum,&leaf);
    file_read_page(table_id,new_leafnum,&new_leaf);
    
    int insertion_index, split, new_key, i, j;

    insertion_index = 0;
    while (insertion_index < leaf.keys){
        slot_t temp;
        memcpy(&temp,leaf.free_space+insertion_index*12,12);
        if(temp.key<pointer->key) insertion_index++;
        else break;
    }
    //insert new slot into extra slot
    for (i = leaf.keys; i > insertion_index; i--) {
        memcpy(leaf.free_space+i*12,leaf.free_space+(i-1)*12,12);
    }
    //put unfinished pointer slot first
    memcpy(leaf.free_space+insertion_index*12,pointer,12);
    
    //set split;
    split = 0;
    int size = 0; //remain leaf size
    for(i = 0; i<leaf.keys+1; i++){
        slot_t temp;
        memcpy(&temp,leaf.free_space+i*12,12);
        if(size+12+temp.val_size<1984) {
            split++;
            size+=(12+temp.val_size);
        }
        else break;
    }

    //move slot - values
    for (i = split, j = 0; i < leaf.keys+1; i++,j++) {
        if(i == insertion_index){
            pointer->offset = 12*new_leaf.keys+new_leaf.free_amount-pointer->val_size;
            memcpy(new_leaf.free_space+j*12,pointer,12);
            new_leaf.keys++;
            memcpy(new_leaf.free_space+pointer->offset,value,pointer->val_size);
            new_leaf.free_amount-=(12+pointer->val_size);
            //delete pointer;
            continue;
        }
        slot_t temp;
        memcpy(&temp, leaf.free_space+i*12,12);
        uint16_t old_offset = temp.offset;
        temp.offset = 12*new_leaf.keys+new_leaf.free_amount-temp.val_size;
        memcpy(new_leaf.free_space+j*12, &temp, 12);
        new_leaf.keys++;
        memcpy(new_leaf.free_space+temp.offset,leaf.free_space+old_offset,temp.val_size);
        new_leaf.free_amount-=(12+temp.val_size);
    }

    //move original keys to temp page
    page_t temp_page;
    temp_page.parent = leaf.parent;
    temp_page.IsLeaf = 1;
    temp_page.keys = 0;
    temp_page.free_amount = 3968;
    
    for(i = 0; i<split; i++){
        if(i == insertion_index){
            pointer->offset = 12*temp_page.keys+temp_page.free_amount-pointer->val_size;
            memcpy(temp_page.free_space+j*12,pointer,12);
            temp_page.keys++;
            memcpy(temp_page.free_space+pointer->offset,value,pointer->val_size);
            temp_page.free_amount-=(12+pointer->val_size);
            //delete pointer;
            continue;
        }
        slot_t temp;
        memcpy(&temp, leaf.free_space+i*12,12);
        uint16_t old_offset = temp.offset;
        temp.offset = 12*temp_page.keys+temp_page.free_amount-temp.val_size;
        memcpy(temp_page.free_space+j*12, &temp, 12);
        temp_page.keys++;
        memcpy(temp_page.free_space+temp.offset,leaf.free_space+old_offset,temp.val_size);
        temp_page.free_amount-=(12+temp.val_size);
    }
    
    new_leaf.next = leaf.next;
    temp_page.next = new_leafnum;
    new_leaf.parent = leaf.parent;
    
    file_write_page(table_id,leafnum,&temp_page);
    file_write_page(table_id,new_leafnum,&new_leaf);
        
    memcpy(&new_key,new_leaf.free_space,8);

    insert_into_parent(table_id, root, leafnum, new_key, new_leafnum);
    delete pointer;
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(int64_t table_id, pagenum_t parent,
        int left_index, uint64_t  key, pagenum_t rightnum) {
    int i;
    page_t parent_info;
    file_read_page(table_id,parent,&parent_info);
    
    for (i = parent_info.keys; i > left_index; i--) {
        memcpy(parent_info.free_space+(i+1)*16,parent_info.free_space+i*16,sizeof(branch_t));
    }
    branch_t temp = {key,rightnum};
    memcpy(parent_info.free_space+left_index*16,&temp,sizeof(branch_t));
    parent_info.keys++;
    file_write_page(table_id,parent,&parent_info);
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
node * insert_into_node_after_splitting(node * root, node * old_node, int left_index, 
        int key, node * right) {

    int i, j, split, k_prime;
    node * new_node, * child;
    int * temp_keys;
    node ** temp_pointers;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_pointers = (node **)malloc( (order + 1) * sizeof(node *) );
    if (temp_pointers == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = (int *)malloc( order * sizeof(int) );
    if (temp_keys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = (node *)old_node->pointers[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  
    split = cut(order);
    new_node = make_node(table_id);
    old_node->num_keys = 0;
    for (i = 0; i < split - 1; i++) {
        old_node->pointers[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
    }
    old_node->pointers[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
    for (++i, j = 0; i < order; i++, j++) {
        new_node->pointers[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->num_keys++;
    }
    new_node->pointers[j] = temp_pointers[i];
    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++) {
        child = (node *)new_node->pointers[i];
        child->parent = new_node;
    }

    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(root, old_node, k_prime, new_node);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
void insert_into_parent(int64_t table_id, pagenum_t root, pagenum_t leftnum, uint64_t key, pagenum_t rightnum) {

    int left_index;
    page_t left;
    file_read_page(table_id,leftnum,&left);
    page_t parent_info;
    pagenum_t parent = left.parent;
    file_read_page(table_id,parent,&parent_info);

    /* Case: new root. */

    if (parent == 0){
        insert_into_new_root(table_id, leftnum, key, rightnum);
        return;
    }

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's pointer to the left 
     * node.
     */

    left_index = 0;
    while (left_index <= parent_info.keys){
        branch_t temp;
        memcpy(&temp,parent_info.free_space+left_index*16,sizeof(branch_t));
        if(temp.pnum != leftnum) left_index++;
        else break;
    }


    /* Simple case: the new key fits into the node. 
     */

    if (parent_info.keys < 248){
        insert_into_node(table_id, parent, left_index, key, rightnum);
        return;
    }

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(root, parent, left_index, key, right);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(int64_t table_id, pagenum_t leftnum, uint64_t key, pagenum_t rightnum) {

    pagenum_t root = make_node(table_id);
    page_t root_info;
    file_read_page(table_id,root,&root_info);
    
    branch_t temp = {key,rightnum};
    root_info.next = leftnum;
    memcpy(&root_info.free_space,&temp,sizeof(branch_t));
    root_info.keys++;
    file_write_page(table_id,root,&root_info);
    
    //update left & right
    page_t left, right;
    file_read_page(table_id,leftnum,&left);
    file_read_page(table_id,rightnum,&right);
    left.parent = root;
    right.parent = root;
    file_write_page(table_id,leftnum,&left);
    file_write_page(table_id,rightnum,&right);
    
    //update header
    page_t header;
    file_read_page(table_id,0,&header);
    memcpy((&header)+16,&root,sizeof(pagenum_t));
    file_write_page(table_id,0,&header);
}



/* First insertion:
 * start a new tree.
 */
void start_new_tree(int64_t table_id, slot_t * pointer, char * value) {

    pagenum_t root = make_node(table_id);
    page_t root_info;
    pagenum_t leafnum = make_leaf(table_id);
    page_t leaf;
    
    //insert to leaf
    file_read_page(table_id,leafnum,&leaf);
    leaf.parent = root;
    //set offset
    pointer->offset = 12*leaf.keys+leaf.free_amount-pointer->val_size;
    memcpy(leaf.free_space+leaf.keys*12,pointer,12);
    leaf.keys++;
    memcpy(leaf.free_space+pointer->offset,value,pointer->val_size);
    leaf.free_amount-=(12+pointer->val_size);
    file_write_page(table_id,leafnum,&leaf);
    
    //set root
    file_read_page(table_id,root,&root_info);
    root_info.next = leafnum;
    file_write_page(table_id,root,&root_info);
    
    //update header
    page_t header;
    file_read_page(table_id,0,&header);
    memcpy((&header)+16,&root,sizeof(pagenum_t));
    file_write_page(table_id,0,&header);
    
    delete pointer;
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
int db_insert( int64_t table_id, int64_t key, char * value, uint16_t val_size ) {
    
    page_t header;
    pagenum_t root;
    file_read_page(table_id,0,&header);
    memcpy(&root,(&header)+16,sizeof(pagenum_t));
    
    slot_t * pointer;
    pagenum_t leafnum;
    page_t leaf;

    /* The current implementation ignores
     * duplicates.
     */

    if (find(table_id, root, key) != NULL){
        perror("Duplicate key error");
        return 1;
    }

    /* Create a new record for the
     * value.
     */
    pointer = make_slot(key,val_size);


    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (root == 0) {
        start_new_tree(key, pointer, value);
        return 0;
    }

    /* Case: the tree already exists.
     * (Rest of function body.)
     */

    leafnum = find_leaf(table_id, root, key);
    file_read_page(table_id,leafnum,&leaf);

    /* Case: leaf has room for key and pointer.
     */

    if (leaf.free_amount < val_size + 12 + 12) { //leave space for extra slot
        insert_into_leaf(table_id, leafnum, pointer, value);
        return 0;
    }


    /* Case:  leaf must be split.
     */

    insert_into_leaf_after_splitting(table_id, root, leafnum, pointer, value);
    return 0;
}




// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( node * n ) {

    int i;

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    for (i = 0; i <= n->parent->num_keys; i++)
        if (n->parent->pointers[i] == n)
            return i - 1;

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int key, node * pointer) {

    int i, num_pointers;

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (n->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
    while (n->pointers[i] != pointer)
        i++;
    for (++i; i < num_pointers; i++)
        n->pointers[i - 1] = n->pointers[i];


    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < order - 1; i++)
            n->pointers[i] = NULL;
    else
        for (i = n->num_keys + 1; i < order; i++)
            n->pointers[i] = NULL;

    return n;
}


node * adjust_root(node * root) {

    node * new_root;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root->num_keys > 0)
        return root;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        new_root = (node *)root->pointers[0];
        new_root->parent = NULL;
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

    free(root->keys);
    free(root->pointers);
    free(root);

    return new_root;
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!n->is_leaf) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pointers[i] = n->pointers[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            tmp = (node *)neighbor->pointers[i];
            tmp->parent = neighbor;
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
        neighbor->pointers[order - 1] = n->pointers[order - 1];
    }

    root = delete_entry(root, n->parent, k_prime, n);
    free(n->keys);
    free(n->pointers);
    free(n); 
    return root;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index, 
        int k_prime_index, int k_prime) {  

    int i;
    node * tmp;

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys];
            tmp = (node *)n->pointers[0];
            tmp->parent = n;
            neighbor->pointers[neighbor->num_keys] = NULL;
            n->keys[0] = k_prime;
            n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
            n->parent->keys[k_prime_index] = n->keys[0];
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            n->parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pointers[n->num_keys + 1] = neighbor->pointers[0];
            tmp = (node *)n->pointers[n->num_keys + 1];
            tmp->parent = n;
            n->parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
            neighbor->pointers[i] = neighbor->pointers[i + 1];
        }
        if (!n->is_leaf)
            neighbor->pointers[i] = neighbor->pointers[i + 1];
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;

    return root;
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
node * delete_entry( node * root, node * n, int key, void * pointer ) {

    int min_keys;
    node * neighbor;
    int neighbor_index;
    int k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, (node *)pointer);

    /* Case:  deletion from the root. 
     */

    if (n == root) 
        return adjust_root(root);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */

    min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (n->num_keys >= min_keys)
        return root;

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    neighbor_index = get_neighbor_index( n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = n->parent->keys[k_prime_index];
    neighbor = neighbor_index == -1 ? (node *)n->parent->pointers[1] : 
        (node *)n->parent->pointers[neighbor_index];

    capacity = n->is_leaf ? order : order - 1;

    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
 */
node * db_delete(node * root, int key) {

    node * key_leaf;
    record * key_record;

    key_record = find(root, key);
    key_leaf = find_leaf(root, key);
    if (key_record != NULL && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, key_record);
        free(key_record);
    }
    return root;
}


void destroy_tree_nodes(node * root) {
    int i;
    if (root->is_leaf)
        for (i = 0; i < root->num_keys; i++)
            free(root->pointers[i]);
    else
        for (i = 0; i < root->num_keys + 1; i++)
            destroy_tree_nodes((node *)root->pointers[i]);
    free(root->pointers);
    free(root->keys);
    free(root);
}


node * destroy_tree(node * root) {
    destroy_tree_nodes(root);
    return NULL;
}
