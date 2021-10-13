#ifndef DB_FILE_H_
#define DB_FILE_H_
#include <page.h>
#define FILE_SIZE (10 * 1024 * 1024)  // 10 MiB

//globals
extern int64_t db_files[19];
extern int db_idx;

// Open existing database file or create one if it doesn't exist
int64_t file_open_database_file(const char* pathname);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id);

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int64_t table_id, pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t pagenum, const page_t* src);

// Close the database file
void file_close_database_file();

#endif  // DB_FILE_H_
