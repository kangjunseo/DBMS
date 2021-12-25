#ifndef DB_FILE_H_
#define DB_FILE_H_

#include "page.h"
#include <atomic>
#include <unordered_map>

using std::unordered_map;

class DiskManager {
	
	private:
		DiskManager(const DiskManager &);
		DiskManager &operator=(const DiskManager &);

	public:
		DiskManager();
		~DiskManager();
		DiskManager(DiskManager&&)=delete;


		int64_t set_table_id(int fd);
		int64_t get_table_id(int fd);
		int get_fd(int64_t table_id);

		void close_table_files();

	private:
		// Table number
		std::atomic<int64_t> max_table_id;

		// Map table id with file descriptor
		unordered_map<int64_t, int> map_tid_to_fd;
};

/** Disk Manager APIs */

int open_disk_manager();
int close_disk_manager();

// Open existing table file or create one if it doesn't exist
int64_t file_open_table_file(const char* pathname);

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id);

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int64_t table_id, pagenum_t pagenum, Page* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t pagenum, const Page* src);

// Close the database file
void file_close_table_files();

#endif /* DB_FILE_H */ 
