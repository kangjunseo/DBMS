#include <stdint.h>
#define PAGE_SIZE 4096
#define FILE_SIZE 10*1024*1024
typedef uint64_t pagenum_t;
struct page_t {
    // in-memory page structure
    pagenum_t next_page_num;
    char reserved[PAGE_SIZE-sizeof(next_page_num)];
};

struct header_page_t {
    // in-memory page structure
    pagenum_t first_page_num;
    pagenum_t pages;
    char reserved[PAGE_SIZE-sizeof(first_page_num)-sizeof(pages)];
};

header_page_t * header; //global variance of header

int db_file; //global variance of db_file

// Open existing database file or create one if not existed.
int file_open_database_file(const char* pathname); 

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int fd);

// Free an on-disk page to the free page list
void file_free_page(int fd, pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int fd, pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(int fd, pagenum_t pagenum, const page_t* src);

// Close the database file
void file_close_database_file();
