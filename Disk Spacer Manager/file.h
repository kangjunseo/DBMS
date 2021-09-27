#include <stdint.h>
#define PAGE_SIZE 4096
typedef uint64_t pagenum_t;
struct page_t {
    // in-memory page structure
    pagenum_t * next_page_t;
    char reserved[PAGE_SIZE-sizeof(next_page_t)];
};

struct header_page_t {
    // in-memory page structure
    pagenum_t * first_page_t;
    pagenum_t pages;
    char reserved[PAGE_SIZE-sizeof(first_page_t)-sizeof(pages)];
};

// Open existing database file or create one if not existed.
int64_t file_open_database_file(char* path);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);

// Stop referencing the database file
void file_close_database_file();
