#ifndef DB_PAGE_H_
#define DB_PAGE_H_

#include <cassert>
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <sys/types.h>

constexpr size_t PG_SIZE = 4096;
constexpr size_t PG_HEADER_SIZE = 128;

using pagenum_t = uint64_t;

// In-memory page structure
struct Meta {
	pagenum_t page_no;
};

// Common Page Header
struct PageHeader {
	pagenum_t parent_page_no;
	uint32_t is_leaf;
	uint32_t num_of_keys;
};

static_assert(sizeof(PageHeader) == 16);

// Physical Page
struct __Page {
	char s[PG_SIZE];
};

// Common In-memory Page
struct Page {
	union {
		pagenum_t next_free_page_no;
		PageHeader header;
		__Page p;
	};

	Meta meta;
};

// Header Page
struct HeaderPage {
	union {
		struct {
			pagenum_t free_page_no;
			uint64_t num_of_pages;
			pagenum_t root_page_no;
		};
		__Page p;
	};

	Meta meta;
};

// Free Page
struct FreePage {
	union {
		pagenum_t next_free_page_no;
		__Page p;
	};

	Meta meta;
};


// Slot record
#pragma pack(push, 2)
struct SlotRecord {
	int64_t key;
	uint16_t size;
	uint16_t off;
};
#pragma pack(pop)

constexpr size_t SLOT_SIZE = 12;
constexpr size_t MIN_VAL_SIZE = 50;
constexpr size_t MAX_VAL_SIZE = 112;

static_assert(sizeof(SlotRecord) == SLOT_SIZE);

// Leaf Page (slotted).
struct LeafPage {
	union {
		struct {
			PageHeader header;
			char __p[96];
			uint64_t free_space;
			uint64_t sibling;
			SlotRecord slots[0];
			char val[0];
		};
		__Page p;
	};

	Meta meta;
};

// Internal record
struct InternalRecord {
	int64_t key;
	pagenum_t value;
};

// Internal Order
constexpr int INTERNAL_ORDER = 249;

// Internal Page
struct InternalPage {
	union {
		struct {
			PageHeader header;
			char __p[96];
			InternalRecord records[INTERNAL_ORDER];
		};
		__Page p;
	};

	Meta meta;
};

constexpr size_t IN_MEM_SIZE = sizeof(Meta);

static_assert(sizeof(InternalPage) == PG_SIZE + IN_MEM_SIZE &&
		sizeof(LeafPage) == PG_SIZE + IN_MEM_SIZE &&
		sizeof(Page) == PG_SIZE + IN_MEM_SIZE);

/** Macros. 'p' should be pointer */
// Page number in-memory
#define GET_PAGE_NO(p) \
	((p)->meta.page_no)
#define SET_PAGE_NO(p, x) \
	((p)->meta.page_no = (x))

// Header Page
#define GET_HEADER_FREE_PAGE_NO(p) \
	((p)->free_page_no)
#define SET_HEADER_FREE_PAGE_NO(p, x) \
	((p)->free_page_no = (x))

#define GET_HEADER_NUM_OF_PAGES(p) \
	((p)->num_of_pages)
#define SET_HEADER_NUM_OF_PAGES(p, x) \
	((p)->num_of_pages = (x))

#define GET_HEADER_ROOT_PAGE_NO(p) \
	((p)->root_page_no)
#define SET_HEADER_ROOT_PAGE_NO(p, x) \
	((p)->root_page_no = (x))

// Free Page
#define GET_FREE_NEXT_PAGE_NO(p) \
	((p)->next_free_page_no)
#define SET_FREE_NEXT_PAGE_NO(p, x) \
	((p)->next_free_page_no = (x))

// Page Header
#define GET_PPAGE_NO(p) \
	((p)->header.parent_page_no)
#define SET_PPAGE_NO(p, x) \
	((p)->header.parent_page_no = (x))

#define GET_IS_LEAF(p) \
	((p)->header.is_leaf)
#define SET_IS_LEAF(p, x) \
	((p)->header.is_leaf = (x))

#define GET_NUM_KEYS(p) \
	((p)->header.num_of_keys)
#define SET_NUM_KEYS(p, x) \
	((p)->header.num_of_keys = (x))

#define INC_NUM_KEYS(p, x) \
	((p)->header.num_of_keys += (x))
#define DEC_NUM_KEYS(p, x) \
	((p)->header.num_of_keys -= (x))

// Leaf Page
#define GET_LEAF_FREE_SPACE(p) \
	((p)->free_space)
#define SET_LEAF_FREE_SPACE(p, x) \
	((p)->free_space = (x))

#define GET_LEAF_SIBLING(p) \
	((p)->sibling)
#define SET_LEAF_SIBLING(p, x) \
	((p)->sibling = x)

#define INC_LEAF_FREE_SPACE(p, x) \
	((p)->free_space += (x))
#define DEC_LEAF_FREE_SPACE(p, x) \
	((p)->free_space -= (x))

#define LEAF_SLOT(p, i) \
	(&(p)->slots[(i)])

#define LEAF_KEY(p, i) \
	((p)->slots[(i)].key)
#define LEAF_VAL(p, i) \
	(&(p)->val[((p)->slots[(i)].off - PG_HEADER_SIZE)])

// Internal Page
#define INTERNAL_KEY(p, i) \
	((p)->records[(i)+1].key)
#define INTERNAL_VAL(p, i) \
	((p)->records[(i)].value)


#endif /* DB_PAGE_H */ 
