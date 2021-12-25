#include "page.h"
#include "file.h"
#include "bpt.h"
#include "msg.h"
#include "buffer.h"

#include <memory>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>

using std::vector;
using std::pair;

/** const variables */
static constexpr uint16_t INIT_FREESPACE = 3968;
static constexpr size_t D_THRES = 2500;
static constexpr size_t I_THRES = 1984;
static constexpr int I_MIN_KEYS = 32;
static constexpr int I_MAX_KEYS = 64;

static_assert(I_THRES == (PG_SIZE - PG_HEADER_SIZE) / 2);
static_assert(INIT_FREESPACE == (PG_SIZE - PG_HEADER_SIZE));

/** static function decl */
static void print_page(
		const std::shared_ptr<Page>& page);
static void print_leaf_page(
		const std::shared_ptr<LeafPage>& p);
static void print_internal_page(
		const std::shared_ptr<InternalPage>& p);

static bool find_leaf(int64_t tid, int64_t key, 
		const std::shared_ptr<LeafPage>& leaf_page);

static bool find_key(int64_t tid, int64_t key, 
		char* ret_val, uint16_t* size,
		const std::shared_ptr<LeafPage>& leaf_page);

static void start_new_tree(int64_t tid, int64_t key,
		char* val, uint16_t size,
		const std::shared_ptr<HeaderPage>& head_page,
		const std::shared_ptr<LeafPage>& leaf_page);

static void insert_into_leaf(int64_t tid, int64_t key,
		char* val, uint16_t size,
		const std::shared_ptr<LeafPage>& leaf_page);

static void insert_into_leaf_after_splitting(
		int64_t tid, int64_t key,
		char* val, uint16_t size,
		const std::shared_ptr<LeafPage>& leaf_page);

static int cut_internal(int length);

static int cut_leaf(const std::shared_ptr<LeafPage>& leaf_page);

static void leaf_page_splitting_internal(
		const std::shared_ptr<LeafPage>& leaf_page,
		const std::shared_ptr<LeafPage>& new_leaf_page,
		int split, int insertion_index,
		int64_t i_key, char* i_val, uint16_t i_size);

static void insert_into_parent(
		const std::shared_ptr<Page>& left,
		const std::shared_ptr<Page>& right,
		int64_t key, int64_t tid);

static void insert_into_new_root(
		const std::shared_ptr<Page>& left,
		const std::shared_ptr<Page>& right,
		int64_t key, int64_t tid);

static int get_left_index(
		const std::shared_ptr<InternalPage>& parent,
		pagenum_t left_page_no);

static void insert_into_internal(
		const std::shared_ptr<InternalPage>& page,
		int left_index, int64_t key, pagenum_t right_page_no);

static void insert_into_internal_after_splitting(
		const std::shared_ptr<InternalPage>& page,
		int left_index, int64_t key, pagenum_t right_page_no,
		int64_t tid);

static void delete_entry(
		const std::shared_ptr<Page>& page,
		int64_t tid, int64_t key);

static void remove_entry_from_page(
		const std::shared_ptr<Page>& page,
		int64_t tid, int64_t key);

static void adjust_root(
		const std::shared_ptr<HeaderPage>& head_page,
		const std::shared_ptr<Page>& page,
		int64_t tid);

static bool delete_done(
		const std::shared_ptr<Page>& page);

static int get_neighbor_index(
		const std::shared_ptr<Page>& page,
		const std::shared_ptr<InternalPage>& parent_page,
		int64_t tid);

static void merge_pages(
		const std::shared_ptr<Page>& page,
		const std::shared_ptr<Page>& neighbor_page,
		int neighbor_index, int64_t k_prime, int64_t tid);

static void redistribute_pages(
		const std::shared_ptr<Page>& page,
		const std::shared_ptr<Page>& neighbor_page,
		int neighbor_index, int k_prime_index, int64_t k_prime, int64_t tid); 


/** static function def */
static void leaf_page_splitting_internal(
		const std::shared_ptr<LeafPage>& leaf_page,
		const std::shared_ptr<LeafPage>& new_leaf_page,
		int split, int insertion_index,
		int64_t i_key, char* i_val, uint16_t i_size) {

	MSG("leaf_page_splitting_internal(). ", 
			"split point:", split, ", insertion:", insertion_index,"\n");

	int i, j;
	SlotRecord* slot;
	SlotRecord* new_slot;
	char* val;
	char* new_val;
	uint16_t off, old_off;
	uint64_t num_of_keys;
	__Page tmp_page;

	assert(GET_NUM_KEYS(new_leaf_page) == 0);
	assert(GET_LEAF_FREE_SPACE(new_leaf_page) == INIT_FREESPACE);
	assert(GET_NUM_KEYS(leaf_page) >= I_MIN_KEYS); 

	if (insertion_index < split) {
		// new key is going to inserted to the old leaf
		
		// Move old leaf to new leaf
		num_of_keys = GET_NUM_KEYS(leaf_page);
		for (i = split, j = 0; i < num_of_keys; i++, j++) {
			// Copy the slot record. 
			memcpy(LEAF_SLOT(new_leaf_page, j), LEAF_SLOT(leaf_page, i), SLOT_SIZE);
			new_slot = LEAF_SLOT(new_leaf_page, j);

			// Find the proper offset.
			off = PG_HEADER_SIZE + 
				GET_NUM_KEYS(new_leaf_page) * SLOT_SIZE + 
				GET_LEAF_FREE_SPACE(new_leaf_page) - new_slot->size;

			assert(off <= PG_SIZE && off >= PG_HEADER_SIZE);
			new_slot->off = off;

			// Copy value
			memcpy(LEAF_VAL(new_leaf_page, j), LEAF_VAL(leaf_page, i),
					(size_t)new_slot->size);

			INC_NUM_KEYS(new_leaf_page, 1);
			DEC_LEAF_FREE_SPACE(new_leaf_page, SLOT_SIZE + new_slot->size);

			DEC_NUM_KEYS(leaf_page, 1);
		}

		// Copy old leaf page to tmp page.
		memcpy(&tmp_page, leaf_page.get(), PG_SIZE);
		val = reinterpret_cast<char*>(&tmp_page);

		// Shift slots
		for (i = split - 1; i >= insertion_index; i--) {
			memcpy(LEAF_SLOT(leaf_page, i + 1), LEAF_SLOT(leaf_page, i), SLOT_SIZE);
		}

		// Insert new slot only (not its value)
		slot = LEAF_SLOT(leaf_page, insertion_index);
		slot->key = i_key;
		slot->size = i_size;

		INC_NUM_KEYS(leaf_page, 1);
		SET_LEAF_FREE_SPACE(leaf_page,
				INIT_FREESPACE - (SLOT_SIZE * GET_NUM_KEYS(leaf_page)));

		off = PG_SIZE;
		/**
		 * For current version, our system doesn't allow any internal fragment
		 * between slots and between values.
		 */
		for (i = 0; i < GET_NUM_KEYS(leaf_page); ++i) {
			slot = LEAF_SLOT(leaf_page, i);
			off -= slot->size;
			old_off = slot->off;
			slot->off = off;

			if (i == insertion_index)
				memcpy(LEAF_VAL(leaf_page, i), i_val, (size_t)slot->size);
			else
				memcpy(LEAF_VAL(leaf_page, i), &val[old_off], (size_t)slot->size);

			DEC_LEAF_FREE_SPACE(leaf_page, slot->size);
		}
	} else {
		// new key is going to inserted to the new leaf
		
		// Set new leaf page with new key and its value
		num_of_keys = GET_NUM_KEYS(leaf_page);
		for (i = split, j = 0; i < num_of_keys; i++, j++) {
			if (i == insertion_index) {
				slot = LEAF_SLOT(new_leaf_page, j);
				slot->key = i_key;
				slot->size = i_size;

				slot->off = PG_HEADER_SIZE + 
					GET_NUM_KEYS(new_leaf_page) * SLOT_SIZE +
					GET_LEAF_FREE_SPACE(new_leaf_page) - slot->size;

				memcpy(LEAF_VAL(new_leaf_page, j), i_val, (size_t)slot->size);
				DEC_LEAF_FREE_SPACE(new_leaf_page, SLOT_SIZE + slot->size);
				INC_NUM_KEYS(new_leaf_page, 1);
				j++;
			}

			memcpy(LEAF_SLOT(new_leaf_page, j), LEAF_SLOT(leaf_page, i), SLOT_SIZE);
			slot = LEAF_SLOT(new_leaf_page, j);
			
			slot->off = PG_HEADER_SIZE +
				GET_NUM_KEYS(new_leaf_page) * SLOT_SIZE +
				GET_LEAF_FREE_SPACE(new_leaf_page) - slot->size;

			memcpy(LEAF_VAL(new_leaf_page, j), 
					LEAF_VAL(leaf_page, i), (size_t)slot->size);

			DEC_NUM_KEYS(leaf_page, 1);
			
			INC_NUM_KEYS(new_leaf_page, 1);
			DEC_LEAF_FREE_SPACE(new_leaf_page, SLOT_SIZE + slot->size);
		}

		// Rare case (right-most insert).
		if (i == insertion_index) {
			slot = LEAF_SLOT(new_leaf_page, j);
			slot->key = i_key;
			slot->size = i_size;

			slot->off = PG_HEADER_SIZE + 
				GET_NUM_KEYS(new_leaf_page) * SLOT_SIZE +
				GET_LEAF_FREE_SPACE(new_leaf_page) - slot->size;

			memcpy(LEAF_VAL(new_leaf_page, j), i_val, (size_t)slot->size);
			DEC_LEAF_FREE_SPACE(new_leaf_page, SLOT_SIZE + slot->size);
			INC_NUM_KEYS(new_leaf_page, 1);
		}

		// Copy old leaf page to tmp page.
		memcpy(&tmp_page, leaf_page.get(), PG_SIZE);
		val = reinterpret_cast<char*>(&tmp_page);

		SET_LEAF_FREE_SPACE(leaf_page,
				INIT_FREESPACE - (SLOT_SIZE * GET_NUM_KEYS(leaf_page)));
		/**
		 * For current version, our system doesn't allow any internal fragment
		 * between slots and between values.
		 */
		off = PG_SIZE;
		for (i = 0; i < GET_NUM_KEYS(leaf_page); ++i) {
			slot = LEAF_SLOT(leaf_page, i);
			off -= slot->size;
			old_off = slot->off;
			slot->off = off;

			memcpy(LEAF_VAL(leaf_page, i), &val[old_off], (size_t)slot->size);
			DEC_LEAF_FREE_SPACE(leaf_page, slot->size);
		}
	}

	// linked-list of leaf pages
	SET_LEAF_SIBLING(new_leaf_page, GET_LEAF_SIBLING(leaf_page));
	SET_LEAF_SIBLING(leaf_page, GET_PAGE_NO(new_leaf_page));

	// Clean-up
	i = GET_NUM_KEYS(leaf_page) * SLOT_SIZE;
	memset(&leaf_page->val[i], 0x00, 
			sizeof(char) * GET_LEAF_FREE_SPACE(leaf_page));

	i = GET_NUM_KEYS(new_leaf_page) * SLOT_SIZE;
	memset(&new_leaf_page->val[i], 0x00, 
			sizeof(char) * GET_LEAF_FREE_SPACE(new_leaf_page));

	// Set parent page no in the new leaf page.
	SET_PPAGE_NO(new_leaf_page, GET_PPAGE_NO(leaf_page));
}

static int cut_internal(int length) {
	if (length % 2 == 0)
		return length / 2;
	else
		return length / 2 + 1;
}

static int cut_leaf(const std::shared_ptr<LeafPage>& leaf_page) {
	uint16_t used_space;
	SlotRecord* slot;
	int i;
	used_space = 0;

	for (i = 0; i < GET_NUM_KEYS(leaf_page); ++i) {
		slot = LEAF_SLOT(leaf_page, i);
		if (used_space + SLOT_SIZE + slot->size >= I_THRES) {
			assert(i != 0);
			break;
		}
		used_space += SLOT_SIZE + slot->size;
	}
	return i;
}

static bool find_leaf(int64_t tid, int64_t key, 
		const std::shared_ptr<LeafPage>& leaf_page) {
	MSG("find_leaf(). ", key, '\n');

	int i;
	
	// Read the header page.
	auto head_page = std::make_shared<HeaderPage>();
	buffer_read_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page));

	// Empty tree.
	if (GET_HEADER_ROOT_PAGE_NO(head_page) == 0) {
		MSG("Empty tree.\n");
		return false;
	}

	// Read the root page.
	auto page = std::make_shared<Page>();
    buffer_read_page(tid, GET_HEADER_ROOT_PAGE_NO(head_page), page);

	while (GET_IS_LEAF(page) != 1) {
		// Find the leaf page.
		auto internal_page = 
			std::reinterpret_pointer_cast<InternalPage>(page);

		i = 0;
		while (i < GET_NUM_KEYS(internal_page)) {
			// Search records in the internal page.
			if (key >= INTERNAL_KEY(internal_page, i)) i++;
			else break;
		}
		
		// Read the child page.
        buffer_read_page(tid, INTERNAL_VAL(internal_page, i), page);
	}

	// Copy
	memcpy(leaf_page.get(), page.get(), sizeof(LeafPage));
	return true;
}

/** Find the record with the given key. */
static bool find_key(int64_t tid, int64_t key, 
		char* ret_val, uint16_t* size, 
		const std::shared_ptr<LeafPage>& leaf_page) {
	MSG("find_key(). ", key, '\n');

	int i;
	SlotRecord* slot;

	// Find the leaf page.
	if (!find_leaf(tid, key, leaf_page)) {
		return false;
	}

	/** Find a given key from the leaf page.
	 * It could be done by binary-search.
	 */
	for (i = 0; i < GET_NUM_KEYS(leaf_page); ++i) {
		if (LEAF_KEY(leaf_page, i) == key) {
			// ret_val should be nullptr in insertion and deletion.
			if (ret_val != nullptr) {
				slot = LEAF_SLOT(leaf_page, i);
				memcpy(ret_val, LEAF_VAL(leaf_page, i), (size_t)slot->size);
				*size = slot->size;
			}
			return true;
		}
	}
	return false;
}


static void start_new_tree(int64_t tid, int64_t key,
		char* val, uint16_t size, 
		const std::shared_ptr<HeaderPage>& head_page,
		const std::shared_ptr<LeafPage>& leaf_page) {
	MSG("start new tree().\n");

	SlotRecord* slot;
	char* rec_val;
	// internally, header page will be changed.
	pagenum_t root_page_no = buffer_alloc_page(tid);
	
	// Read the header page again. see buffer_alloc_page().
    buffer_read_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page));
	// Read the new root page by root_page_no..
    buffer_read_page(tid, root_page_no,
			std::reinterpret_pointer_cast<Page>(leaf_page));

	memset(leaf_page.get(), 0x00, sizeof(LeafPage));
	
	// Initialize values.
	SET_PAGE_NO(leaf_page, root_page_no);
	SET_PPAGE_NO(leaf_page, 0);
	SET_IS_LEAF(leaf_page, 1);
	SET_NUM_KEYS(leaf_page, 1);
	SET_LEAF_FREE_SPACE(leaf_page, INIT_FREESPACE - SLOT_SIZE - size);

	// Set the first slot and its value.
	slot = LEAF_SLOT(leaf_page, 0);

	slot->key = key;
	slot->off = (uint16_t)(PG_SIZE - size);
	slot->size = size;

	memcpy(LEAF_VAL(leaf_page, 0), val, (size_t)size);

	// Set the root page number in the header page
	SET_HEADER_ROOT_PAGE_NO(head_page, root_page_no);
    MSG("ROOT NO : ",head_page->root_page_no,' ',root_page_no,'\n');
    buffer_write_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page).get());
    buffer_write_page(tid, root_page_no,
                      std::reinterpret_pointer_cast<Page>(leaf_page).get());
}

static void insert_into_leaf(
		int64_t tid, int64_t key, 
		char* val, uint16_t size,
		const std::shared_ptr<LeafPage>& leaf_page) {

	MSG("insert_into_leaf(). ", key, ' ', size, '\n');

	int insertion_point;
	int i;
	uint16_t off;
	SlotRecord* slot;
	uint32_t num_of_keys;
	uint64_t free_space;

	num_of_keys = GET_NUM_KEYS(leaf_page);
	assert(num_of_keys <= I_MAX_KEYS);
	free_space = GET_LEAF_FREE_SPACE(leaf_page);
	
	// Find the proper offset.
	off = PG_HEADER_SIZE + 
		SLOT_SIZE * num_of_keys + 
		free_space - size;

	assert(off <= PG_SIZE && off >= PG_HEADER_SIZE);
	assert(size >= MIN_VAL_SIZE && size <= MAX_VAL_SIZE);

	insertion_point = 0;
	// Find insertion point.
	while (insertion_point < num_of_keys &&
			LEAF_KEY(leaf_page, insertion_point) < key)
		insertion_point++;

	// Shift slots.
	for (i = num_of_keys - 1; i >= insertion_point; --i) {
		memcpy(LEAF_SLOT(leaf_page, i + 1), LEAF_SLOT(leaf_page, i), SLOT_SIZE);
	}

	// Insert new slot and its value.
	slot = LEAF_SLOT(leaf_page, insertion_point);
	slot->key = key;
	slot->size = size;
	slot->off = off;

	memcpy(LEAF_VAL(leaf_page, insertion_point), val, (size_t)size);

	DEC_LEAF_FREE_SPACE(leaf_page, SLOT_SIZE + size);
	INC_NUM_KEYS(leaf_page, 1);	

    buffer_write_page(tid, GET_PAGE_NO(leaf_page),
                      std::reinterpret_pointer_cast<Page>(leaf_page).get());
}

static void insert_into_leaf_after_splitting(
		int64_t tid, int64_t key,
		char* val, uint16_t size,
		const std::shared_ptr<LeafPage>& leaf_page) {

	MSG("insert_into_leaf_after_splitting(). ", key, ' ', size, '\n');

	int insertion_index, split;
	int64_t new_key;

	// Make the new leaf page.
	auto new_leaf_page = std::make_shared<LeafPage>();
	memset(new_leaf_page.get(), 0x00, sizeof(LeafPage));
	
	// Initialize values
	SET_PAGE_NO(new_leaf_page, buffer_alloc_page(tid));
	SET_LEAF_FREE_SPACE(new_leaf_page, INIT_FREESPACE);
	SET_IS_LEAF(new_leaf_page, 1);
	SET_NUM_KEYS(new_leaf_page, 0);

	insertion_index = 0;
	// Find insertion index.
	while (insertion_index < GET_NUM_KEYS(leaf_page) && 
			LEAF_KEY(leaf_page, insertion_index) < key)
		insertion_index++;

	// Find the split index based on its free space.
	split = cut_leaf(leaf_page);

	// Move slots and their values to new leaf page.
	leaf_page_splitting_internal(
			leaf_page, new_leaf_page, split, insertion_index,
			key, val, size);

    buffer_write_page(tid, GET_PAGE_NO(leaf_page),
                      std::reinterpret_pointer_cast<Page>(leaf_page).get());
    buffer_write_page(tid, GET_PAGE_NO(new_leaf_page),
                      std::reinterpret_pointer_cast<Page>(new_leaf_page).get());

	new_key = LEAF_KEY(new_leaf_page, 0);

	// Insert new key and new leaf to the parent
	insert_into_parent(
			std::reinterpret_pointer_cast<Page>(leaf_page),
			std::reinterpret_pointer_cast<Page>(new_leaf_page),
			new_key, tid);
}

static void insert_into_parent(
		const std::shared_ptr<Page>& left,
		const std::shared_ptr<Page>& right,
		int64_t key, int64_t tid) {

	MSG("insert_into_parent(). ", key, '\n');

	pagenum_t parent_page_no;
	auto parent_page = std::make_shared<InternalPage>();

	parent_page_no = GET_PPAGE_NO(
			std::reinterpret_pointer_cast<LeafPage>(left));

	// Make new root.
	if (parent_page_no == 0) {
		insert_into_new_root(left, right, key, tid);
		return;
	}

    buffer_read_page(tid, parent_page_no,
			std::reinterpret_pointer_cast<Page>(parent_page));

	// Find the parent's pointer to the left page.
	int left_index = 
		get_left_index(parent_page, GET_PAGE_NO(left));

	/* Simple case: the new key fits into the node. 
	*/
	if (GET_NUM_KEYS(parent_page) < INTERNAL_ORDER - 1) {
		insert_into_internal(parent_page, left_index, key, GET_PAGE_NO(right));
        buffer_write_page(tid, parent_page_no,
                          std::reinterpret_pointer_cast<Page>(parent_page).get());
		return;
	}

	/* Harder case:  split a node in order 
	 * to preserve the B+ tree properties.
	 */
	return insert_into_internal_after_splitting(
			parent_page, left_index, key, GET_PAGE_NO(right), tid);
}

static void insert_into_new_root(
		const std::shared_ptr<Page>& left,
		const std::shared_ptr<Page>& right,
		int64_t key, int64_t tid) {

	MSG("insert_into_new_root(). ", key, '\n');

	// Get the new root page.
	auto new_root_page = std::make_shared<InternalPage>();
	memset(new_root_page.get(), 0x00, sizeof(InternalPage));

	SET_PAGE_NO(new_root_page, buffer_alloc_page(tid));
    
	INTERNAL_KEY(new_root_page, 0) = key;
	INTERNAL_VAL(new_root_page, 0) = GET_PAGE_NO(left);
	INTERNAL_VAL(new_root_page, 1) = GET_PAGE_NO(right);

	SET_NUM_KEYS(new_root_page, 1);
	SET_PPAGE_NO(new_root_page, 0);
	SET_IS_LEAF(new_root_page, 0);

	SET_PPAGE_NO(left, GET_PAGE_NO(new_root_page));
	SET_PPAGE_NO(right, GET_PAGE_NO(new_root_page));

    buffer_write_page(tid, GET_PAGE_NO(new_root_page),
                      std::reinterpret_pointer_cast<Page>(new_root_page).get());

    buffer_write_page(tid, GET_PAGE_NO(left), left.get());
    buffer_write_page(tid, GET_PAGE_NO(right), right.get());

	// Set root page number in the header page.
	auto head_page = std::make_shared<HeaderPage>();
    buffer_read_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page));
	SET_HEADER_ROOT_PAGE_NO(head_page, GET_PAGE_NO(new_root_page));

    buffer_write_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page).get());
	
}

static int get_left_index(
		const std::shared_ptr<InternalPage>& parent, pagenum_t left_page_no) {

	int left_index = 0;
	while (left_index <= GET_NUM_KEYS(parent) && 
			INTERNAL_VAL(parent, left_index) != left_page_no)
		left_index++;
	return left_index;
}

static void insert_into_internal(
		const std::shared_ptr<InternalPage>& page, 
		int left_index, int64_t key, pagenum_t right_page_no) {
	MSG("insert_into_internal(). ", key, ' ', right_page_no, '\n');

	int i;

	for (i = GET_NUM_KEYS(page); i > left_index; i--) {
		INTERNAL_VAL(page, i + 1) = INTERNAL_VAL(page, i);
		INTERNAL_KEY(page, i) = INTERNAL_KEY(page, i - 1);
	}
	INTERNAL_VAL(page, left_index + 1) = right_page_no;
	INTERNAL_KEY(page, left_index) = key;

	INC_NUM_KEYS(page, 1);
}

static void insert_into_internal_after_splitting(
		const std::shared_ptr<InternalPage>& old_page,
		int left_index, int64_t key, pagenum_t right_page_no,
		int64_t tid) {
	MSG("insert_into_internal_after_splitting(). ", key, ' ', right_page_no, '\n');

	int i, j, split, k_prime;

	/* First create a temporary set of keys and pointers
	 * to hold everything in order, including
	 * the new key and pointer, inserted in their
	 * correct places. 
	 * Then create a new node and copy half of the 
	 * keys and pointers to the old node and
	 * the other half to the new.
	 */
	auto temp_pointers = std::make_unique<pagenum_t[]>(INTERNAL_ORDER + 1);
	auto temp_keys = std::make_unique<int64_t[]>(INTERNAL_ORDER);

	for (i = 0, j = 0; i < GET_NUM_KEYS(old_page) + 1; i++, j++) {
		if (j == left_index + 1) j++;
		temp_pointers.get()[j] = INTERNAL_VAL(old_page, i);
	}

	for (i = 0, j = 0; i < GET_NUM_KEYS(old_page); i++, j++) {
		if (j == left_index) j++;
		temp_keys.get()[j] = INTERNAL_KEY(old_page, i);
	}

	temp_pointers.get()[left_index + 1] = right_page_no;
	temp_keys.get()[left_index] = key;

	/* Create the new node and copy
	 * half the keys and pointers to the
	 * old and half to the new.
	 */  
	auto new_page = std::make_shared<InternalPage>();
	split = cut_internal(INTERNAL_ORDER);

	SET_PAGE_NO(new_page, buffer_alloc_page(tid));
	SET_NUM_KEYS(new_page, 0);
	SET_IS_LEAF(new_page, 0);
	SET_PPAGE_NO(new_page, GET_PPAGE_NO(old_page));

	SET_NUM_KEYS(old_page, 0);

	for (i = 0; i < split - 1; i++) {
		INTERNAL_VAL(old_page, i) = temp_pointers.get()[i];
		INTERNAL_KEY(old_page, i) = temp_keys.get()[i];
		INC_NUM_KEYS(old_page, 1);
	}

	INTERNAL_VAL(old_page, i) = temp_pointers.get()[i];
	k_prime = temp_keys.get()[split - 1];

	for (++i, j = 0; i < INTERNAL_ORDER; i++, j++) {
		INTERNAL_VAL(new_page, j) = temp_pointers.get()[i];
		INTERNAL_KEY(new_page, j) = temp_keys.get()[i];
		INC_NUM_KEYS(new_page, 1);
	}

	INTERNAL_VAL(new_page, j) = temp_pointers.get()[i];

	// Leaf or Internal.
	auto child_page = std::make_shared<Page>();

	for (i = 0; i <= GET_NUM_KEYS(new_page); i++) {
        buffer_read_page(tid, INTERNAL_VAL(new_page, i), child_page);
		SET_PPAGE_NO(child_page, GET_PAGE_NO(new_page));
        buffer_write_page(tid, INTERNAL_VAL(new_page, i), child_page.get());
	}

	// clear garbage record
	for (i = GET_NUM_KEYS(old_page); i < INTERNAL_ORDER - 1; i++) {
		INTERNAL_VAL(old_page, i + 1) = 0;
		INTERNAL_KEY(old_page, i) = 0;
	}

	for (i = GET_NUM_KEYS(new_page); i < INTERNAL_ORDER - 1; i++) {
		INTERNAL_VAL(new_page, i + 1) = 0;
		INTERNAL_KEY(new_page, i) = 0;
	}

    buffer_write_page(tid, GET_PAGE_NO(new_page),
                      std::reinterpret_pointer_cast<Page>(new_page).get());
    buffer_write_page(tid, GET_PAGE_NO(old_page),
                      std::reinterpret_pointer_cast<Page>(old_page).get());

	/* Insert a new key into the parent of the two
	 * nodes resulting from the split, with
	 * the old node to the left and the new to the right.
	 */
	insert_into_parent(
			std::reinterpret_pointer_cast<Page>(old_page),
			std::reinterpret_pointer_cast<Page>(new_page),
			k_prime, tid);
}

static void remove_entry_from_page(
		const std::shared_ptr<Page>& page,
		int64_t tid, int64_t key) {

	int i;
	int key_idx = 0;

	if (GET_IS_LEAF(page) == 1) {
		// Leaf Page.
		SlotRecord* slot;
		char* val;
		char* new_val;
		uint16_t d_size, d_off;
		auto leaf_page = std::reinterpret_pointer_cast<LeafPage>(page);
		vector<pair<int, uint16_t>> tmp_list{};

		/**
		 * Keep all records' (execept record to be deleted) index and offset 
		 * for compacting a leaf page.
		 */
		for (i = 0; i < GET_NUM_KEYS(leaf_page); i++) {
			if (LEAF_KEY(leaf_page, i) == key) {
				slot = LEAF_SLOT(leaf_page, i);
				d_size = slot->size;
				d_off = slot->off;
				key_idx = i;
				break;
			}
			slot = LEAF_SLOT(leaf_page, i);
			tmp_list.emplace_back(i, slot->off);
		}

		// Shift slot records
		for (i = key_idx; i < GET_NUM_KEYS(leaf_page) - 1; i++) {
			memcpy(LEAF_SLOT(leaf_page, i), LEAF_SLOT(leaf_page, i + 1), SLOT_SIZE);
			slot = LEAF_SLOT(leaf_page, i);
			tmp_list.emplace_back(i, slot->off);
		}

		// Sort list of <index (slot rec), offset >
		std::sort(tmp_list.begin(), tmp_list.end(),
				[] (const auto& x, const auto& y) -> bool {
					return x.second > y.second;
				});

		// Compact values
		for (const auto& x : tmp_list) {
			if (x.second > d_off)
				continue;
			
			slot = LEAF_SLOT(leaf_page, x.first);
			val = LEAF_VAL(leaf_page, x.first);
			slot->off += d_size;
			new_val = LEAF_VAL(leaf_page, x.first);

			memmove(new_val, val, (size_t)slot->size);
		}
				
		DEC_NUM_KEYS(leaf_page, 1);
		INC_LEAF_FREE_SPACE(leaf_page, SLOT_SIZE + d_size);

		// Clean-up
		i = GET_NUM_KEYS(leaf_page) * SLOT_SIZE;
		memset(&leaf_page->val[i], 0x00,
				sizeof(char) * GET_LEAF_FREE_SPACE(leaf_page));

	} else {
		// Internal page.
		auto internal_page = std::reinterpret_pointer_cast<InternalPage>(page);

		// find a slot of deleting key
		for (i = 0; i < GET_NUM_KEYS(internal_page); i++) {
			if (INTERNAL_KEY(internal_page, i) == key) {
				key_idx = i;
				break;
			}
		}

		assert(i != GET_NUM_KEYS(internal_page));

		// shift keys/pointers
		for (i = key_idx; i < GET_NUM_KEYS(internal_page) - 1; i++) {
			INTERNAL_KEY(internal_page, i) = INTERNAL_KEY(internal_page, i + 1);
			INTERNAL_VAL(internal_page, i + 1) = INTERNAL_VAL(internal_page, i + 2);
		}
		// clear garbage key/pointer
		INTERNAL_KEY(internal_page, i) = 0;
		INTERNAL_VAL(internal_page, i + 1) = 0;

		DEC_NUM_KEYS(internal_page, 1);
	}

    buffer_write_page(tid, GET_PAGE_NO(page), page.get());
}

static void adjust_root(
		const std::shared_ptr<HeaderPage>& head_page,
		const std::shared_ptr<Page>& page,
		int64_t tid) {
	MSG("adjust_root().\n");

	/** Non-empty root. */
	if (GET_NUM_KEYS(page) > 0)
		return;

	if (GET_IS_LEAF(page) == 0) {
		/** Make the first child as the root. */
		auto internal_page = std::reinterpret_pointer_cast<InternalPage>(page);
		SET_HEADER_ROOT_PAGE_NO(head_page, INTERNAL_VAL(internal_page, 0));

		auto new_root_page = std::make_shared<Page>();
        buffer_read_page(
				tid, GET_HEADER_ROOT_PAGE_NO(head_page), new_root_page);

		SET_PPAGE_NO(new_root_page, 0);
		
        buffer_write_page(
				tid, GET_PAGE_NO(new_root_page), new_root_page.get());

        buffer_write_page(tid, 0,
                          std::reinterpret_pointer_cast<Page>(head_page).get());
	} else {
		/** Whole tree is empty. */
		SET_HEADER_ROOT_PAGE_NO(head_page, 0);
        buffer_write_page(tid, 0,
                          std::reinterpret_pointer_cast<Page>(head_page).get());
	}

    buffer_free_page(tid, GET_PAGE_NO(page));
}

static void delete_entry(
		const std::shared_ptr<Page>& page,
		int64_t tid, int64_t key) {
	MSG("delete_entry(). key : ", key, '\n');
	pagenum_t neighbor_page_no;
	int neighbor_index;
	int k_prime_index;
	int64_t k_prime;
	int capacity;

	auto head_page = std::make_shared<HeaderPage>();
    buffer_read_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page));

	// Remove key and value from page.
	remove_entry_from_page(page, tid, key);

	/** Delete in the root page. */
	if (GET_HEADER_ROOT_PAGE_NO(head_page) == GET_PAGE_NO(page)) {
		adjust_root(head_page, page, tid);
		return;
	}

	/** If true, return immediately. */
	if (delete_done(page))
		return;

	/** Page should be merged or redistributed. */

	/* Find the appropriate neighbor page  with which
	 * to merge.
	 * Also find the key (k_prime) in the parent
	 * between the pointer to page n and the pointer
	 * to the neighbor.
	 */
	auto parent_page = std::make_shared<InternalPage>();
	neighbor_index = get_neighbor_index(page, parent_page, tid);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;

	k_prime = INTERNAL_KEY(parent_page, k_prime_index);

	neighbor_page_no = neighbor_index == -1 ? INTERNAL_VAL(parent_page, 1) : 
		INTERNAL_VAL(parent_page, neighbor_index);

	auto neighbor_page = std::make_shared<Page>();
    buffer_read_page(tid, neighbor_page_no, neighbor_page);

	if (GET_IS_LEAF(page) == 1) {
		// Leaf page.
		if (GET_LEAF_FREE_SPACE(
					std::reinterpret_pointer_cast<LeafPage>(page)) +
				GET_LEAF_FREE_SPACE(
					std::reinterpret_pointer_cast<LeafPage>(neighbor_page)) >=
				INIT_FREESPACE) {
			merge_pages(page, neighbor_page, neighbor_index, k_prime, tid);
		} else {
			redistribute_pages(page, neighbor_page, neighbor_index, 
					k_prime_index, k_prime, tid);
		}
	} else {
		// Internal page.
		if (GET_NUM_KEYS(page) + GET_NUM_KEYS(neighbor_page) < INTERNAL_ORDER - 1) {
			merge_pages(page, neighbor_page, neighbor_index, k_prime, tid);
		} else {
			redistribute_pages(page, neighbor_page, neighbor_index, 
					k_prime_index, k_prime, tid);
		}
	}
}

static void merge_pages(
		const std::shared_ptr<Page>& page,
		const std::shared_ptr<Page>& neighbor_page,
		int neighbor_index, int64_t k_prime, int64_t tid) {

	MSG("merge_pages(). k_prime : ", k_prime, '\n');

	int i, j, neighbor_insertion_index, n_end;

	/* Starting point in the neighbor for copying
	 * keys and pointers from n.
	 * Recall that n and neighbor have swapped places
	 * in the special case of n being a leftmost child.
	 */

	/* Case:  nonleaf node.
	 * Append k_prime and the following pointer.
	 * Append all pointers and keys from the neighbor.
	 */
	if (GET_IS_LEAF(page) != 1) {
		auto i_page = std::reinterpret_pointer_cast<InternalPage>(page);
		auto i_neighbor_page = 
			std::reinterpret_pointer_cast<InternalPage>(neighbor_page);
		/* Swap neighbor with node if node is on the
		 * extreme left and neighbor is to its right.
		 */
		if (neighbor_index == -1) {
			i_page.swap(i_neighbor_page);
		}

		neighbor_insertion_index = GET_NUM_KEYS(i_neighbor_page);

		/* Append k_prime. */
		INTERNAL_KEY(i_neighbor_page, neighbor_insertion_index) = k_prime;
		INC_NUM_KEYS(i_neighbor_page, 1);

		n_end = GET_NUM_KEYS(i_page);

		for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
			INTERNAL_KEY(i_neighbor_page, i) = INTERNAL_KEY(i_page, j);
			INTERNAL_VAL(i_neighbor_page, i) = INTERNAL_VAL(i_page, j);
			INC_NUM_KEYS(i_neighbor_page, 1);
			DEC_NUM_KEYS(i_page, 1);
		}

		/* The number of pointers is always
		 * one more than the number of keys.
		 */
		INTERNAL_VAL(i_neighbor_page, i) = INTERNAL_VAL(i_page, j);

		/* All children must now point up to the same parent.
		*/
		auto child_page = std::make_shared<Page>();
		for (i = 0; i < GET_NUM_KEYS(i_neighbor_page) + 1; i++) {
            buffer_read_page(tid, INTERNAL_VAL(i_neighbor_page, i), child_page);
			SET_PPAGE_NO(child_page, GET_PAGE_NO(i_neighbor_page));
            buffer_write_page(tid, INTERNAL_VAL(i_neighbor_page, i), child_page.get());
		}

        buffer_write_page(tid, GET_PAGE_NO(i_neighbor_page),
                          std::reinterpret_pointer_cast<Page>(i_neighbor_page).get());
        buffer_free_page(tid, GET_PAGE_NO(i_page));
	} else {
		/* In a leaf, append the keys and pointers of
		 * n to the neighbor.
		 * Set the neighbor's last pointer to point to
		 * what had been n's right neighbor.
		 */
		SlotRecord* slot;
		uint64_t current_off;

		auto l_page = std::reinterpret_pointer_cast<LeafPage>(page);
		auto l_neighbor_page = 
			std::reinterpret_pointer_cast<LeafPage>(neighbor_page);

		/* Swap neighbor with the page if page is on the
		 * extreme left and neighbor is to its right.
		 */
		if (neighbor_index == -1) {
			l_page.swap(l_neighbor_page);
		}

		neighbor_insertion_index = GET_NUM_KEYS(l_neighbor_page);

		current_off = PG_HEADER_SIZE + 
			GET_NUM_KEYS(l_neighbor_page) * SLOT_SIZE +
			GET_LEAF_FREE_SPACE(l_neighbor_page);

		for (i = neighbor_insertion_index, j = 0; 
				j < GET_NUM_KEYS(l_page); i++, j++) {
			memcpy(LEAF_SLOT(l_neighbor_page, i), LEAF_SLOT(l_page, j), SLOT_SIZE);
			slot = LEAF_SLOT(l_neighbor_page, i);
			current_off -= slot->size;
			slot->off = current_off;

			memcpy(LEAF_VAL(l_neighbor_page, i), 
					LEAF_VAL(l_page, j), (size_t)slot->size);

			INC_NUM_KEYS(l_neighbor_page, 1);
			assert(GET_LEAF_FREE_SPACE(l_neighbor_page) >= SLOT_SIZE + slot->size);
			DEC_LEAF_FREE_SPACE(l_neighbor_page, SLOT_SIZE + slot->size);
		}

		SET_LEAF_SIBLING(l_neighbor_page, GET_LEAF_SIBLING(l_page));

        buffer_write_page(tid, GET_PAGE_NO(l_neighbor_page),
                          std::reinterpret_pointer_cast<Page>(l_neighbor_page).get());
		buffer_free_page(tid, GET_PAGE_NO(l_page));
	}

	auto parent_page = std::make_shared<Page>();
    buffer_read_page(tid, GET_PPAGE_NO(neighbor_page), parent_page);
	delete_entry(parent_page, tid, k_prime);
}

static void redistribute_pages(
		const std::shared_ptr<Page>& page,
		const std::shared_ptr<Page>& neighbor_page,
		int neighbor_index, int k_prime_index, int64_t k_prime, int64_t tid) {

	MSG("redistribute_pages(). k_prime : ", k_prime, '\n');
	/** helper function for compacting values in the leaf page. */
	auto help_func = [](const auto& m_list, uint16_t off) -> uint16_t {
		uint16_t move_size = 0;
		for (const auto& x: m_list) {
			if (x.first < off)
				break;
			else
				move_size += x.second;
		}
		return move_size;
	};

	int i;

	/* Case: n has a neighbor to the left. 
	 * Pull the neighbor's last key-pointer pair over
	 * from the neighbor's right end to n's left end.
	 */
	if (neighbor_index != -1) {
		if (GET_IS_LEAF(page) != 1) {
			// Internal page.
			auto i_page = std::reinterpret_pointer_cast<InternalPage>(page);
			auto i_neighbor_page = 
				std::reinterpret_pointer_cast<InternalPage>(neighbor_page);

			INTERNAL_VAL(i_page, GET_NUM_KEYS(i_page) + 1) = 
				INTERNAL_VAL(i_page, GET_NUM_KEYS(i_page));

			for (i = GET_NUM_KEYS(i_page); i > 0; i--) {
				INTERNAL_KEY(i_page, i) = INTERNAL_KEY(i_page, i - 1);
				INTERNAL_VAL(i_page, i) = INTERNAL_VAL(i_page, i - 1);
			}

			INTERNAL_VAL(i_page, 0) = 
				INTERNAL_VAL(i_neighbor_page, GET_NUM_KEYS(i_neighbor_page));
			
			// Set child-page's parent page no.
			auto child_page = std::make_shared<Page>();
            buffer_read_page(tid, INTERNAL_VAL(i_page, 0), child_page);
			SET_PPAGE_NO(child_page, GET_PAGE_NO(i_page));
            buffer_write_page(tid, INTERNAL_VAL(i_page, 0), child_page.get());

			INTERNAL_VAL(i_neighbor_page, GET_NUM_KEYS(i_neighbor_page)) = 0;
			INTERNAL_KEY(i_page, 0) = k_prime;

			auto parent_page = std::make_shared<InternalPage>();
            buffer_read_page(tid, GET_PPAGE_NO(i_page),
					std::reinterpret_pointer_cast<Page>(parent_page));

			INTERNAL_KEY(parent_page, k_prime_index) = 
				INTERNAL_KEY(i_neighbor_page, GET_NUM_KEYS(i_neighbor_page) - 1);

            buffer_write_page(tid, GET_PAGE_NO(parent_page),
                              std::reinterpret_pointer_cast<Page>(parent_page).get());

			/* n now has one more key and one more pointer;
			 * the neighbor has one fewer of each.
			 */
			INC_NUM_KEYS(i_page, 1);
			DEC_NUM_KEYS(i_neighbor_page, 1);

            buffer_write_page(tid, GET_PAGE_NO(page), page.get());
            buffer_write_page(tid, GET_PAGE_NO(neighbor_page), neighbor_page.get());
		} else {
			// Leaf page.
			int move_cnt = 0;
			uint64_t l_page_free_space, l_neighbor_page_free_space;
			SlotRecord* slot;
			SlotRecord* new_slot;

			char* val;
			char* new_val;

			vector<pair<int, uint16_t>> tmp_list{};
			vector<pair<uint16_t, uint16_t>> moved_list{};

			auto l_page = std::reinterpret_pointer_cast<LeafPage>(page);
			auto l_neighbor_page = 
				std::reinterpret_pointer_cast<LeafPage>(neighbor_page);

			// neighbor(left) -> page (right)
			l_page_free_space = GET_LEAF_FREE_SPACE(l_page);
			l_neighbor_page_free_space = 
				GET_LEAF_FREE_SPACE(l_neighbor_page);

			assert(l_page_free_space >= D_THRES);
			assert(l_neighbor_page_free_space < D_THRES);

			/** Find the number of records to be moved. */
			for (i = GET_NUM_KEYS(l_neighbor_page) - 1; i >= 0; --i) {
				if (l_page_free_space < D_THRES)
					break;
				move_cnt++;
				slot = LEAF_SLOT(l_neighbor_page, i);
				l_page_free_space -= slot->size + SLOT_SIZE;
				l_neighbor_page_free_space += slot->size + SLOT_SIZE;
			}

			assert(move_cnt <= 2 && move_cnt != 0);
			assert(l_page_free_space < D_THRES);
			assert(l_neighbor_page_free_space < D_THRES);
		
			/** Shift slots in the leaf page. */
			for (i = GET_NUM_KEYS(l_page) - 1; i >= 0; --i) {
				memcpy(LEAF_SLOT(l_page, i + move_cnt), 
									LEAF_SLOT(l_page, i), SLOT_SIZE);
			}

			/** Copy slot(s) in neighbor to the target page. */
			memcpy(LEAF_SLOT(l_page, 0),
					LEAF_SLOT(l_neighbor_page, GET_NUM_KEYS(l_neighbor_page) - move_cnt),
					SLOT_SIZE * move_cnt);

			/** Increse num_keys in the target page, and decrease the free space. */ 
			INC_NUM_KEYS(l_page, move_cnt);
			DEC_LEAF_FREE_SPACE(l_page, SLOT_SIZE * move_cnt); // Slot only

			/** Copy the value and decrease the free space. */
			for (i = 0; i < move_cnt; ++i) {
				slot = LEAF_SLOT(
						l_neighbor_page, GET_NUM_KEYS(l_neighbor_page) - move_cnt + i);

				/** Save its offset and size for compact. */
				moved_list.emplace_back(slot->off, slot->size);

				slot = LEAF_SLOT(l_page, i);

				slot->off =
					PG_HEADER_SIZE +
					SLOT_SIZE * GET_NUM_KEYS(l_page)  +
					GET_LEAF_FREE_SPACE(l_page) - slot->size;
				
				memcpy(LEAF_VAL(l_page, i), LEAF_VAL(
							l_neighbor_page, GET_NUM_KEYS(l_neighbor_page) - move_cnt + i),
							(size_t)slot->size);

				DEC_LEAF_FREE_SPACE(l_page, slot->size);
			}

			/** Sort the moved list by offset value. */
			std::sort(moved_list.begin(), moved_list.end(),
					[] (const auto& x, const auto& y) -> bool {
						return x.first > y.first;
					});

			/** The target page is done. */
			assert(l_page_free_space == GET_LEAF_FREE_SPACE(l_page));
			
			/** Save its index and offset for compact. */
			for (i = 0; i < GET_NUM_KEYS(l_neighbor_page) - move_cnt; ++i) {
				slot = LEAF_SLOT(l_neighbor_page, i);
				tmp_list.emplace_back(i, slot->off);
			}

			/** Sort the tmp list by offset value. */
			std::sort(tmp_list.begin(), tmp_list.end(),
					[] (const auto& x, const auto& y) -> bool {
						return x.second > y.second;
					});

			/** Compact values. */
			for (const auto& x: tmp_list) {
				if (x.second > moved_list[0].first)
					continue;

				slot = LEAF_SLOT(l_neighbor_page, x.first);
				val = LEAF_VAL(l_neighbor_page, x.first);
				slot->off += help_func(moved_list, slot->off);
				new_val = LEAF_VAL(l_neighbor_page, x.first);

				memmove(new_val, val, (size_t)slot->size);
			}

			/** Decrease num_keys, and increase the free space. */
			DEC_NUM_KEYS(l_neighbor_page, move_cnt);
			INC_LEAF_FREE_SPACE(l_neighbor_page, move_cnt * SLOT_SIZE + 
					help_func(moved_list, 0));

			/** The neighbor page is done. */
			assert(l_neighbor_page_free_space ==
					GET_LEAF_FREE_SPACE(l_neighbor_page));

			/** Modify the value in the parent page. */
			auto parent_page = std::make_shared<InternalPage>();
            buffer_read_page(tid, GET_PPAGE_NO(l_page),
					std::reinterpret_pointer_cast<Page>(parent_page));
			INTERNAL_KEY(parent_page, k_prime_index) = LEAF_KEY(l_page, 0);
            buffer_write_page(tid, GET_PPAGE_NO(l_page),
                              std::reinterpret_pointer_cast<Page>(parent_page).get());

            buffer_write_page(tid, GET_PAGE_NO(page), page.get());
            buffer_write_page(tid, GET_PAGE_NO(neighbor_page), neighbor_page.get());
		}
	} else {  
		/* Case: n is the leftmost child.
		 * Take a key-pointer pair from the neighbor to the right.
		 * Move the neighbor's leftmost key-pointer pair
		 * to n's rightmost position.
		 */
		if (GET_IS_LEAF(page) == 1) {
			// Leaf Page
			int move_cnt = 0;
			uint64_t l_page_free_space, l_neighbor_page_free_space;
			SlotRecord* slot;
			SlotRecord* new_slot;

			char* val;
			char* new_val;

			vector<pair<int, uint16_t>> tmp_list{};
			vector<pair<uint16_t, uint16_t>> moved_list{};

			auto l_page = std::reinterpret_pointer_cast<LeafPage>(page);
			auto l_neighbor_page = 
				std::reinterpret_pointer_cast<LeafPage>(neighbor_page);

			// neighbor(right) -> page (left)
			l_page_free_space = GET_LEAF_FREE_SPACE(l_page);
			l_neighbor_page_free_space = 
				GET_LEAF_FREE_SPACE(l_neighbor_page);

			assert(l_page_free_space >= D_THRES);
			assert(l_neighbor_page_free_space < D_THRES);

			/** Find the number of records to be moved. */
			for (i = 0; i < GET_NUM_KEYS(l_neighbor_page); ++i) {
				if (l_page_free_space < D_THRES)
					break;
				move_cnt++;
				slot = LEAF_SLOT(l_neighbor_page, i);
				l_page_free_space -= slot->size + SLOT_SIZE;
				l_neighbor_page_free_space += slot->size + SLOT_SIZE;
			}

			assert(move_cnt <= 2 && move_cnt != 0);
			assert(l_page_free_space < D_THRES);
			assert(l_neighbor_page_free_space < D_THRES);

			/** Copy slot(s) in neighbor to the target page. */
			memcpy(LEAF_SLOT(l_page, GET_NUM_KEYS(l_page)),
						LEAF_SLOT(l_neighbor_page, 0),
						SLOT_SIZE * move_cnt);

			/** Increse num_keys in the target page, and decrease the free space. */ 
			INC_NUM_KEYS(l_page, move_cnt);
			DEC_LEAF_FREE_SPACE(l_page, SLOT_SIZE * move_cnt);

			/** Copy the value and decrease the free space. */
			for (i = 0; i < move_cnt; ++i) {
				slot = LEAF_SLOT(l_neighbor_page, i);

				/** Save its offset and size for compact. */
				moved_list.emplace_back(slot->off, slot->size);

				slot = LEAF_SLOT(l_page,
						GET_NUM_KEYS(l_page) - move_cnt + i);

				slot->off =
					PG_HEADER_SIZE +
					SLOT_SIZE * GET_NUM_KEYS(l_page)  +
					GET_LEAF_FREE_SPACE(l_page) - slot->size;
				
				memcpy(LEAF_VAL(l_page, GET_NUM_KEYS(l_page) + i - move_cnt), 
						LEAF_VAL(l_neighbor_page, i),
						(size_t)slot->size);

				DEC_LEAF_FREE_SPACE(l_page, slot->size);
			}

			/** Sort the moved list by offset value. */
			std::sort(moved_list.begin(), moved_list.end(),
					[] (const auto& x, const auto& y) -> bool {
						return x.first > y.first;
					});

			/** The target page is done. */
			assert(l_page_free_space == GET_LEAF_FREE_SPACE(l_page));
			
			/** Compact slots and, save its index and offset for compact. */
			for (i = move_cnt; i < GET_NUM_KEYS(l_neighbor_page); ++i) {
				memcpy(LEAF_SLOT(l_neighbor_page, i - move_cnt),
						LEAF_SLOT(l_neighbor_page, i),
						SLOT_SIZE);
				slot = LEAF_SLOT(l_neighbor_page, i);
				tmp_list.emplace_back(i - move_cnt, slot->off);
			}

			/** Decrease num_keys */
			DEC_NUM_KEYS(l_neighbor_page, move_cnt);


			/** Sort the tmp list by offset value. */
			std::sort(tmp_list.begin(), tmp_list.end(),
					[] (const auto& x, const auto& y) -> bool {
						return x.second > y.second;
					});

			/** Compact values. */
			for (const auto& x: tmp_list) {
				if (x.second > moved_list[0].first)
					continue;

				slot = LEAF_SLOT(l_neighbor_page, x.first);
				val = LEAF_VAL(l_neighbor_page, x.first);
				slot->off += help_func(moved_list, slot->off);
				new_val = LEAF_VAL(l_neighbor_page, x.first);

				memmove(new_val, val, (size_t)slot->size);
			}

			/** Increase the free space. */
			INC_LEAF_FREE_SPACE(l_neighbor_page, move_cnt * SLOT_SIZE + 
					help_func(moved_list, 0));

			/** The neighbor page is done. */
			assert(l_neighbor_page_free_space ==
					GET_LEAF_FREE_SPACE(l_neighbor_page));

			/** Modify the value in the parent page. */
			auto parent_page = std::make_shared<InternalPage>();
            buffer_read_page(tid, GET_PPAGE_NO(l_page),
					std::reinterpret_pointer_cast<Page>(parent_page));
			INTERNAL_KEY(parent_page, k_prime_index) = LEAF_KEY(l_neighbor_page, 0);
            buffer_write_page(tid, GET_PPAGE_NO(l_page),
                              std::reinterpret_pointer_cast<Page>(parent_page).get());

            buffer_write_page(tid, GET_PAGE_NO(page), page.get());
            buffer_write_page(tid, GET_PAGE_NO(neighbor_page), neighbor_page.get());

		} else {
			// Internal Page
			auto i_page = std::reinterpret_pointer_cast<InternalPage>(page);
			auto i_neighbor_page = 
				std::reinterpret_pointer_cast<InternalPage>(neighbor_page);

			INTERNAL_KEY(i_page, GET_NUM_KEYS(i_page)) = k_prime;
			INTERNAL_VAL(i_page, GET_NUM_KEYS(i_page) + 1) = 
				INTERNAL_VAL(i_neighbor_page, 0);

			auto child_page = std::make_shared<Page>();
            buffer_read_page(tid,
					INTERNAL_VAL(i_page, GET_NUM_KEYS(i_page) + 1), child_page);

			SET_PPAGE_NO(child_page, GET_PAGE_NO(i_page));
            buffer_write_page(tid,
					INTERNAL_VAL(i_page, GET_NUM_KEYS(i_page) + 1), child_page.get());

			auto parent_page = std::make_shared<InternalPage>();
            buffer_read_page(tid, GET_PPAGE_NO(i_page),
					std::reinterpret_pointer_cast<Page>(parent_page));

			INTERNAL_KEY(parent_page, k_prime_index) = 
				INTERNAL_KEY(i_neighbor_page, 0);

            buffer_write_page(tid, GET_PPAGE_NO(i_page),
                              std::reinterpret_pointer_cast<Page>(parent_page).get());

			for (i = 0; i < GET_NUM_KEYS(i_neighbor_page) - 1; i++) {
				INTERNAL_KEY(i_neighbor_page, i) = INTERNAL_KEY(i_neighbor_page, i + 1);
				INTERNAL_VAL(i_neighbor_page, i) = INTERNAL_VAL(i_neighbor_page, i + 1);
			}

			INTERNAL_VAL(i_neighbor_page, i) = INTERNAL_VAL(i_neighbor_page, i + 1);


			/* n now has one more key and one more pointer;
			 * the neighbor has one fewer of each.
			 */
			INC_NUM_KEYS(i_page, 1);
			DEC_NUM_KEYS(i_neighbor_page, 1);

            buffer_write_page(tid, GET_PAGE_NO(page), page.get());
            buffer_write_page(tid, GET_PAGE_NO(neighbor_page), neighbor_page.get());
		}
	}
}

/** Check whether additional work(merge, redistribute) is needed or not. */
static bool delete_done(
		const std::shared_ptr<Page>& page) {
	
	if (GET_IS_LEAF(page) == 1) {
		// Leaf Page
		auto leaf_page = std::reinterpret_pointer_cast<LeafPage>(page);

		if (GET_LEAF_FREE_SPACE(leaf_page) >= D_THRES) {
			return false;
		} else {
			return true;
		}
	} else {
		// Internal Page
		auto internal_page =
			std::reinterpret_pointer_cast<InternalPage>(page);

		if (GET_NUM_KEYS(internal_page) >= cut_internal(INTERNAL_ORDER) - 1) {
			return true;
		} else {
			return false;
		}
	}
}

static int get_neighbor_index(
		const std::shared_ptr<Page>& page,
		const std::shared_ptr<InternalPage>& parent_page,
		int64_t tid) {

	int i;

	/* Return the index of the key to the left
	 * of the pointer in the parent pointing
	 * to n.  
	 * If n is the leftmost child, this means
	 * return -1.
	 */
    buffer_read_page(tid, GET_PPAGE_NO(page),
			std::reinterpret_pointer_cast<Page>(parent_page));

	for (i = 0; i <= GET_NUM_KEYS(parent_page); i++)
		if (INTERNAL_VAL(parent_page, i) == GET_PAGE_NO(page))
			return i - 1;

	// Error state.
	assert(false);
	return -1;
}

/** non-static function def */

/*
 * int find_record()
 * @param[in]				tid : table id returned from open table
 * @param[in]				key : key value
 * @param[in/out]		ret_val : return value
 * @param[in/out]		size : size of ret_val. (variable-length)
 * @return: if success, return 0. Else return non-zero.
 */
int find_record(int64_t tid, 
		int64_t key, 
		char* ret_val,
		uint16_t* size) {
	MSG("[BEGIN] find_record(). ", key, '\n');
	auto leaf_page = std::make_shared<LeafPage>();

	/** Find key. */
	if (!find_key(tid, key, ret_val, size, leaf_page)) {
		MSG("[END] fail\n");
		return -1;
	}
	MSG("[END] success\n");
	return 0;
}

/*
 * int insert_record()
 * @param[in]		tid : table id returned from open table
 * @param[in]		key : key value
 * @param[in]		val : value
 * @param[in]		size : size of val (var-length)
 * @return: if success, return 0. Else return non-zero
 */
int insert_record(int64_t tid,
		int64_t key, 
		char* val,
		uint16_t size) {
	
	MSG("[BEGIN] insert_record(). ", key, ' ', size, '\n');
	auto leaf_page = std::make_shared<LeafPage>();
	auto head_page = std::make_shared<HeaderPage>();

	if (find_key(tid, key, nullptr, nullptr, leaf_page)) {
		// Duplicated key.
		MSG("[END] dup key\n");
		return -1;
	}

	// Read the header page.
    buffer_read_page(tid, 0, std::reinterpret_pointer_cast<Page>(head_page));
    MSG("ROOT NO : ",head_page->root_page_no,'\n');
	// Empty tree.
	if (GET_HEADER_ROOT_PAGE_NO(head_page) == 0) {
		start_new_tree(tid, key, val, size, head_page, leaf_page);
		MSG("[END] success\n");
		return 0;
	}

	if (GET_LEAF_FREE_SPACE(leaf_page) >= SLOT_SIZE + size) {
		// Enough space for insertion.
		insert_into_leaf(tid, key, val, size, leaf_page);
	} else {
		// No room for insertion. Do split.
		insert_into_leaf_after_splitting(tid, key, val, size, leaf_page);
	}

	MSG("[END] success\n");
	return 0;
}

/*
 * int delete_record()
 * @param[in]		tid : table id returned from open table
 * @param[in]		key : key value
 * @return: if success, return 0. Else return non-zero
 */
int delete_record(int64_t tid, int64_t key) {
	MSG("[BEGIN] delete_record(). ", key, '\n');

	auto leaf_page = std::make_shared<LeafPage>();

	// Find key
	if (!find_key(tid, key, nullptr, nullptr, leaf_page)) {
		MSG("[END] No key.\n");
		return -1;
	}

	delete_entry(std::reinterpret_pointer_cast<Page>(leaf_page), tid, key);
	MSG("[END] success\n");
	return 0;
}

#ifdef DBG_PRINT
/** Print page for debug */
static void print_page(
		const std::shared_ptr<Page>& page) {
	if (GET_IS_LEAF(page) == 1)
		print_leaf_page(
				std::reinterpret_pointer_cast<LeafPage>(page));
	else
		print_internal_page(
				std::reinterpret_pointer_cast<InternalPage>(page));
}

static void print_leaf_page(
		const std::shared_ptr<LeafPage>& p) {
	std::cerr << "[PRINT] page no : " << GET_PAGE_NO(p) << '\n';
	std::cerr << "[HEADER]\n";
	std::cerr << "PPage no 	: " << GET_PPAGE_NO(p) << ", ";
	std::cerr << "Is Leaf  	: " << GET_IS_LEAF(p) << ", ";
	std::cerr << "# of Key 	: " << GET_NUM_KEYS(p) << '\n';
	std::cerr << "Freespace	: " << GET_LEAF_FREE_SPACE(p) << ", ";
	std::cerr << "Sibling  	: " << GET_LEAF_SIBLING(p) << '\n';
	std::cerr << "[SLOTS]\n";
	SlotRecord* slot = nullptr;

	for (int i = 0; i < GET_NUM_KEYS(p); ++i) {
		slot = LEAF_SLOT(p, i);
		std::cerr << "[ " << i << " slot record] ";
		std::cerr << "key   : " << slot->key << ", ";
		std::cerr << "size  : " << slot->size << ", ";
		char val[MAX_VAL_SIZE];
		memset(val, 0x00, sizeof(char) * MAX_VAL_SIZE);
		std::cerr << "off		: " << slot->off << '\n';
		memcpy(val, LEAF_VAL(p, i), slot->size);
		std::cerr << "                 ";
		std::cerr << "val   : " << val << '\n';
	}
	std::cerr << "[PRINT DONE]\n";
}

static void print_internal_page(
		const std::shared_ptr<InternalPage>& p) {
	std::cerr << "[PRINT] page no : " << GET_PAGE_NO(p) << '\n';
	std::cerr << "[HEADER]\n";
	std::cerr << "PPage no 	: " << GET_PPAGE_NO(p) << ", ";
	std::cerr << "Is Leaf  	: " << GET_IS_LEAF(p) << ", ";
	std::cerr << "# of Key 	: " << GET_NUM_KEYS(p) << '\n';
	std::cerr << "[Records]\n";

	for (int i = 0; i < GET_NUM_KEYS(p) + 1; ++i) {
		std::cerr << "[ " << i << " record] ";
		std::cerr << "key 	: " << INTERNAL_KEY(p, i - 1) << ", ";
		std::cerr << "val		: " << INTERNAL_VAL(p, i) << '\n';
	}
	std::cerr << "[PRINT DONE]\n";
}


#else
static void print_page(
		const std::shared_ptr<Page>& page) {
	return;
}

static void print_leaf_page(
		const std::shared_ptr<LeafPage>& leaf_page) {
	return;
}
static void print_internal_page(
		const std::shared_ptr<InternalPage>& internal_page) {
	return;
}

#endif /* DBG_PRINT */

