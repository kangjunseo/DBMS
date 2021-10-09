#ifndef DB_PAGE_H_
#define DB_PAGE_H_
#include <stdint.h>
#define PAGE_SIZE 4096  // 4 KB

typedef uint64_t pagenum_t;

struct page_t{
  //in-memory page structure

  //page_header
  pagenum_t parent;
  uint32_t IsLeaf;  // 0 : internal, 1 : leaf
  uint32_t keys;
  char reserved[96];
  uint64_t free_amount; //default 3968, only for leaf
  pagenum_t next; // right sibling pagenum(leaf), left most pagenum(internal)

  //slots & values(leaf) or Branching factors(internal)
  char free_space [3968];
};

#pragma pack(push, 1)
struct slot_t{  // for leaf
  uint64_t key;
  uint16_t val_size;
  uint16_t offset;
};
#pragma pack(pop)

struct branch_t{  // for internal
  uint64_t key;
  uint64_t pnum;
};

struct free_page_t {
  pagenum_t next_page_num;
  char reserved[PAGE_SIZE-sizeof(next_page_num)];
};

struct header_page_t{
  pagenum_t first_page_num;
  pagenum_t pages;
  pagenum_t root;
  char reserved[PAGE_SIZE-sizeof(first_page_num)-sizeof(pages)-sizeof(root)];
};
#endif  //DB_PAGE_H_
