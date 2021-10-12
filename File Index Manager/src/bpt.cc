/*
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 */

#include "bpt.h"

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

// FIND.


/* Traces the path from the root to a leaf, searching
 * by key. Returns the leaf containing the given key.
 */
pagenum_t find_leaf( int64_t table_id, pagenum_t root, int64_t key ) {
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
slot_t * find( int64_t table_id, pagenum_t root, int64_t key ) {
    int i = 0;
    page_t * c =  new page_t;
    if(c == NULL){
        perror("find error");
        exit(EXIT_FAILURE);
    }
    file_read_page(table_id,find_leaf( table_id, root, key ),c);
    
    slot_t * temp = new slot_t;
    if(temp == NULL){
        perror("find error");
        exit(EXIT_FAILURE);
    }
    
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

int db_find(int64_t table_id, int64_t key, char * ret_val, uint16_t * val_size){
    //find root
    page_t header, target;
    pagenum_t root;
    file_read_page(table_id,0,&header);
    memcpy(&root,header.reserved,sizeof(pagenum_t));
    
    slot_t * slot = new slot_t;
    if(slot == NULL){
        perror("db_find slot creation error");
        exit(EXIT_FAILURE);
    }
    
    pagenum_t targetnum = find_leaf(table_id, root , key);
    slot = find(table_id, root, key);
    if(slot == NULL) return -1;
    
    file_read_page(table_id,targetnum,&target);
    memcpy(ret_val,target.free_space+slot->offset,slot->val_size);
    memcpy(val_size,&slot->val_size,sizeof(uint16_t));
    
    return 0;
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

    insert_into_parent(table_id, leafnum, new_key, new_leafnum);
    delete pointer;
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
void insert_into_node(int64_t table_id, pagenum_t parent,
        int left_index, int64_t  key, pagenum_t rightnum) {
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
void insert_into_node_after_splitting(int64_t table_id, pagenum_t old_nodenum, int left_index,
        int64_t key, pagenum_t rightnum) {

    int i, j, split;
    int64_t k_prime;
    
    page_t temp, new_node;
    file_read_page(table_id,old_nodenum,&temp);
    branch_t insert={key,rightnum};
    
    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    branch_t carrier[249]; //temp carrier of branches
    
    for (i = 0, j = 0; i < temp.keys; i++, j++) {
        if (j == left_index) j++;
        memcpy(&carrier[j],temp.free_space+i*sizeof(branch_t),sizeof(branch_t));
    }
    memcpy(&carrier[left_index],&insert,sizeof(branch_t));

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  
    split = 124;
    pagenum_t new_nodenum = make_node(table_id);
    file_read_page(table_id,new_nodenum,&new_node);
    temp.keys = 0;
    
    for (i = 0; i < split; i++) {
        memcpy(temp.free_space+i*sizeof(branch_t),&carrier[i],sizeof(branch_t));
        temp.keys++;
    }
    
    memcpy(&new_node.next,&carrier[i].pnum,sizeof(pagenum_t));
    k_prime = carrier[split].key;
    
    for (++i, j = 0; i < 249; i++, j++) {
        memcpy(new_node.free_space+j*sizeof(branch_t),&carrier[i],sizeof(branch_t));
        new_node.keys++;
    }
    
    new_node.parent = temp.parent;
    
    for (i = 0; i <= new_node.keys; i++) {
        page_t temp;
        pagenum_t tempnum;
        
        if(i==0) memcpy(&tempnum,&new_node.next,sizeof(pagenum_t));
        else memcpy(&tempnum,new_node.free_space+i*16-8,sizeof(pagenum_t));
        
        file_read_page(table_id,tempnum,&temp);
        temp.parent = new_nodenum;
        file_write_page(table_id,tempnum,&temp);
    }
    
    file_write_page(table_id,old_nodenum,&temp);
    file_write_page(table_id,new_nodenum,&new_node);
    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    insert_into_parent(table_id, old_nodenum, k_prime, new_nodenum);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
void insert_into_parent(int64_t table_id, pagenum_t leftnum, int64_t key, pagenum_t rightnum) {

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

    insert_into_node_after_splitting(table_id, parent, left_index, key, rightnum);
}

/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
void insert_into_new_root(int64_t table_id, pagenum_t leftnum, int64_t key, pagenum_t rightnum) {

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
    memcpy(header.reserved,&root,sizeof(pagenum_t));
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
    memcpy(header.reserved,&root,sizeof(pagenum_t));
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
    memcpy(&root,header.reserved,sizeof(pagenum_t));
    
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
int get_neighbor_index( int64_t table_id, pagenum_t n ) {

    int i;
    page_t temp, parent_info;
    file_read_page(table_id,n,&temp);
    pagenum_t parent = temp.parent;
    file_read_page(table_id,parent,&parent_info);
    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
    pagenum_t iter = parent_info.next;
    for (i = 0; i <= parent_info.keys; i++){
        if (iter == n)
            return i - 1;
        memcpy(&iter,parent_info.free_space+i*16-8,sizeof(pagenum_t));
    }

    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}

bool cmp_slot(const slot_t & b1, const slot_t & b2){ //sort slot by offset
  if(b1.offset>b2.offset) return true; //descending order
  return false;
}

void remove_entry_from_node(int64_t table_id, pagenum_t n, int64_t key) {

    int i,j;
    page_t target;
    file_read_page(table_id,n,&target);
    // Remove the key and shift other keys accordingly.
    i = 0;
    j = 0;
    if(target.IsLeaf){ //leaf page
        slot_t carrier[64];
        uint16_t target_offset, target_val_size;
        for(;i<target.keys;i++,j++){  //make slot carrier and delete target slot
            memcpy(&carrier[j],target.free_space+i*12,12);
            memcpy(target.free_space+j*12,&carrier[j],12);
            if(carrier[j].key == key) {
                j--;
                target_offset = carrier[j].offset;
                target_val_size = carrier[j].val_size;
            }
        }
        target.keys--;
        std::sort(carrier,carrier+target.keys,cmp_slot); //for keep compacted
        
        for(i=0;i<target.keys;i++){ //delete target value and keep compact
            if(carrier[i].offset < target_offset)
                memcpy(target.free_space+carrier[i].offset+target_val_size,target.free_space+carrier[i].offset,carrier[i].val_size);
        }
        target.free_amount-=(12+target_val_size);
    }else{  //internal page
        branch_t target_branch;
        memcpy(&target_branch,target.free_space,sizeof(branch_t));
        i = 0;
        while(target_branch.key != key){
            i++;
            memcpy(&target_branch,target.free_space+16*i,sizeof(branch_t));
        }
        for(++i;i<target.keys;i++){
            memcpy(target.free_space+(i-1)*16,target.free_space+i*16,sizeof(branch_t));
        }
        target.keys--;
    }
    file_write_page(table_id,n,&target);
}


void adjust_root(int64_t table_id) {

    page_t header, root_info;
    pagenum_t root;
    file_read_page(table_id,0,&header);
    memcpy(&root,header.reserved,sizeof(pagenum_t));
    file_read_page(table_id,root,&root_info);
    
    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */

    if (root_info.keys > 0)
        return;

    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.
    pagenum_t new_root = root_info.next;
    file_read_page(table_id,new_root,&root_info);
    if(root_info.IsLeaf) new_root = 0;
    else root_info.parent = 0;
    
    memcpy(header.reserved,&new_root,sizeof(pagenum_t));
    file_free_page(table_id,root);
    file_write_page(table_id,0,&header);
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
void coalesce_nodes(int64_t table_id, pagenum_t n, pagenum_t neighbor,
                      int neighbor_index, int64_t k_prime) {

    page_t neighbor_info, target;
    int i, j, neighbor_insertion_index, n_end;
    pagenum_t tmp;
    
    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

    file_read_page(table_id,neighbor,&neighbor_info);
    file_read_page(table_id,n,&target);
    
    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor_info.keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!target.IsLeaf) {

        /* Append k_prime.
         */
        branch_t k_prime_branch = {k_prime,target.next};
        
        if(neighbor_insertion_index == 0)  // might not happen noramally
            memcpy(&neighbor_info.next,&target.next,8);
        else
            memcpy(neighbor_info.free_space+neighbor_insertion_index*16,&k_prime_branch,16);
        neighbor_info.keys++;

        n_end = target.keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            memcpy(neighbor_info.free_space+i*16,target.free_space+j*16,16);
            neighbor_info.keys++;
            target.keys--;
        }

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor_info.keys + 1; i++) {
            pagenum_t temp;
            page_t child;
            if(i==0) memcpy(&temp,&neighbor_info.next,8);
            else memcpy(&temp,neighbor_info.free_space+i*16-8,8);
            
            file_read_page(table_id,temp,&child);
            child.parent = neighbor;
            file_write_page(table_id,temp,&child);
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < target.keys; i++, j++) {
            slot_t temp;
            memcpy(&temp,target.free_space+j*12,12);
            uint16_t old_offset = temp.offset;
            temp.offset = 12*neighbor_info.keys + neighbor_info.free_amount - temp.val_size;
            
            memcpy(neighbor_info.free_space+i*12,&temp,12);
            memcpy(neighbor_info.free_space+temp.offset,target.free_space+old_offset,temp.val_size);
            neighbor_info.keys++;
            neighbor_info.free_amount-=(12+temp.val_size);
        }
        
    }
    file_write_page(table_id,neighbor,&neighbor_info);
    file_free_page(table_id,n);
    delete_entry(table_id, target.parent, k_prime);
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
void redistribute_nodes(int64_t table_id, pagenum_t n, pagenum_t neighbor, int neighbor_index,int k_prime_index, int64_t k_prime) {

    int i;
    page_t target, neighbor_info;
    file_read_page(table_id,n,&target);
    file_read_page(table_id,neighbor,&neighbor_info);
    
    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!target.IsLeaf){
            for (i = target.keys; i > 0; i--) {
                memcpy(target.free_space+i*16,target.free_space+(i-1)*16,16);
            }
            branch_t temp = {k_prime , target.next};
            memcpy(target.free_space,&temp,16);
            memcpy(&target.next,neighbor_info.free_space+16*neighbor_info.keys-8,8);
            
            page_t page;
            file_read_page(table_id,target.next,&page);
            page.parent = n;
            file_write_page(table_id,target.next,&page);
            
            file_read_page(table_id,target.parent,&page);
            memcpy(page.free_space+16*k_prime_index,neighbor_info.free_space+(neighbor_info.keys-2)*16,8);
            file_write_page(table_id,target.parent,&page);
            
            target.keys++;
            neighbor_info.keys--;
            
            file_write_page(table_id,n,&target);
            file_write_page(table_id,neighbor,&neighbor_info);
        }
        else { //leaf
            while(target.free_amount >= 2500){
                slot_t temp;
                for(int i=target.keys;i>0;i--){
                    memcpy(target.free_space+i*12,target.free_space+(i-1)*12,12);
                }
                memcpy(&temp, neighbor_info.free_space+(neighbor_info.keys-1)*12,12);
                uint16_t old_offset = temp.offset;
                temp.offset = 12*target.keys + target.free_amount - temp.val_size;
                
                memcpy(target.free_space,&temp,12);
                memcpy(target.free_space+temp.offset,neighbor_info.free_space+old_offset,temp.val_size);
                
                target.keys++;
                neighbor_info.keys--;
                target.free_amount-=(12+temp.val_size);
                neighbor_info.free_amount+=(12+temp.val_size);
            }
            page_t page;
            file_read_page(table_id,target.parent,&page);
            memcpy(page.free_space+16*k_prime_index,target.free_space,8);
            file_write_page(table_id,target.parent,&page);
            
            //keep neighbor compacted
            file_read_page(table_id,neighbor,&page);
            page.keys = 0;
            page.free_amount = 3968;
            memcpy(page.free_space,0,3968);
            
            for(int i=0;i<neighbor_info.keys;i++){
                slot_t temp;
                memcpy(&temp,neighbor_info.free_space+i*12,12);
                uint16_t old_offset = temp.offset;
                temp.offset = 12*page.keys + page.free_amount - temp.val_size;
                memcpy(page.free_space,&temp,12);
                memcpy(page.free_space+temp.offset,neighbor_info.free_space+old_offset,temp.val_size);
                
                page.keys++;
                page.free_amount-=(12+temp.val_size);
            }
            
            file_write_page(table_id,neighbor,&page);
            file_write_page(table_id,n,&target);
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {
        if(!target.IsLeaf){
            branch_t temp = {k_prime,neighbor_info.next};
            memcpy(target.free_space+target.keys*16,&temp,16);
            
            page_t page;
            file_read_page(table_id,neighbor_info.next,&page);
            page.parent = n;
            file_write_page(table_id,neighbor_info.next,&page);
            
            file_read_page(table_id,target.parent,&page);
            memcpy(page.free_space+16*k_prime_index,neighbor_info.free_space,8);
            file_write_page(table_id,target.parent,&page);
            
            for(i = 0; i< neighbor_info.keys-1; i++){
                memcpy(neighbor_info.free_space+i*16,neighbor_info.free_space+(i+1)*16,16);
            }
            
            target.keys++;
            neighbor_info.keys--;
        
            file_write_page(table_id,n,&target);
            file_write_page(table_id,neighbor,&neighbor_info);
        }
        else { // leaf
            while(target.free_amount >= 2500){
                slot_t temp;
                memcpy(&temp,neighbor_info.free_space,12);
                uint16_t old_offset = temp.offset;
                temp.offset = 12*target.keys + target.free_amount - temp.val_size;
                memcpy(target.free_space+12*target.keys,&temp,12);
                memcpy(target.free_space+temp.offset,neighbor_info.free_space+ old_offset,temp.val_size);
                
                for(int i=0;i<neighbor_info.keys-1;i++){
                    memcpy(neighbor_info.free_space+i*12,neighbor_info.free_space+(i+1)*12,12);
                }
                
                target.keys++;
                neighbor_info.keys--;
                target.free_amount-=(12+temp.val_size);
                neighbor_info.free_amount+=(12+temp.val_size);
            }
            page_t page;
            file_read_page(table_id,target.parent,&page);
            memcpy(page.free_space+16*k_prime_index,neighbor_info.free_space,8);
            file_write_page(table_id,target.parent,&page);
            
            //keep neighbor compacted
            file_read_page(table_id,neighbor,&page);
            page.keys = 0;
            page.free_amount = 3968;
            memcpy(page.free_space,0,3968);
            
            for(int i=0;i<neighbor_info.keys;i++){
                slot_t temp;
                memcpy(&temp,neighbor_info.free_space+i*12,12);
                uint16_t old_offset = temp.offset;
                temp.offset = 12*page.keys + page.free_amount - temp.val_size;
                memcpy(page.free_space,&temp,12);
                memcpy(page.free_space+temp.offset,neighbor_info.free_space+old_offset,temp.val_size);
                
                page.keys++;
                page.free_amount-=(12+temp.val_size);
            }
            
            file_write_page(table_id,neighbor,&page);
            file_write_page(table_id,n,&target);
        }
    }

}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
void delete_entry( int64_t table_id, pagenum_t n, int64_t key ) {

    page_t header, target, parent_info, neighbor_info;
    pagenum_t neighbor, root, parent;
    int neighbor_index, k_prime_index;
    int64_t k_prime;
    file_read_page(table_id,0,&header);
    memcpy(&root,header.reserved,sizeof(pagenum_t));

    // Remove key and pointer from node.

    remove_entry_from_node(table_id, n, key);

    /* Case:  deletion from the root. 
     */

    if (n == root) {
        adjust_root(table_id);
        return;
    }

    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */
    
    file_read_page(table_id,n,&target);
    parent = target.parent;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if(target.IsLeaf){
        if (target.free_amount < 2500) return;
    }else{
        if(target.keys >= 124 ) return;
    }
    
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

    neighbor_index = get_neighbor_index( table_id, n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    memcpy(&k_prime, parent_info.free_space+16*k_prime_index,sizeof(int64_t));
    if(neighbor_index == -1) memcpy(&neighbor,parent_info.free_space+8,8);
    else memcpy(&neighbor,parent_info.free_space+neighbor_index*16-8,8);
    
    file_read_page(table_id,neighbor,&neighbor_info);
    
    /* Coalescence or Redistribution. */
    
    if(target.IsLeaf){
        if (neighbor_info.free_amount + target.free_amount >= 3968 + 12){ //leave space for extra slot(in insert)
            coalesce_nodes(table_id, n, neighbor, neighbor_index, k_prime);
        }else{
            redistribute_nodes(table_id, n, neighbor, neighbor_index, k_prime_index, k_prime);
        }
    }else{
        if(neighbor_info.keys + target.keys < 248){
            coalesce_nodes(table_id, n, neighbor, neighbor_index, k_prime);
        }else{
            redistribute_nodes(table_id, n, neighbor, neighbor_index, k_prime_index, k_prime);
        }
    }
}



/* Master deletion function.
 */
int db_delete(int64_t table_id, int64_t key) {
    
    page_t header;
    pagenum_t root;
    file_read_page(table_id,0,&header);
    memcpy(&root,header.reserved,sizeof(pagenum_t));

    pagenum_t key_leafnum = 0;
    slot_t * key_record = find(table_id,root,key);
    key_leafnum = find_leaf(table_id, root, key);
    
    if (key_record != NULL && key_leafnum != 0) {
        delete_entry(root, key_leafnum, key);
    }
    return 0;
}
