#include "file.h"
#include "msg.h"

#include <fcntl.h>
#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <cstring>
#include <unistd.h>

/** const vars */
constexpr int DEFAULT_SIZE = 10 * 1024 * 1024; /** 10MiB */
constexpr int DEFAULT_NUM_OF_PAGES = DEFAULT_SIZE / PG_SIZE;
static_assert(DEFAULT_NUM_OF_PAGES == 2560);

/** DiskManager */
static std::unique_ptr<DiskManager> disk_manager {nullptr};

/** static funtion decl */
static void init_header_page(HeaderPage*);
static void file_read_page_internal(int, off_t, void*);
static void file_write_page_internal(int, off_t, const void*);
static pagenum_t file_expand_file(int, HeaderPage*);


/** DiskManager Class APIs */
DiskManager::DiskManager() {
	this->max_table_id = 1;
	this->map_tid_to_fd.clear();
}

DiskManager::~DiskManager() {
	this->map_tid_to_fd.clear();
}

/** Return new table id matching with the given file descriptor. */
int64_t DiskManager::set_table_id(int fd) {
	int64_t ret_table_id;
	ret_table_id = this->max_table_id++;
	if (this->map_tid_to_fd.find(ret_table_id) !=
			this->map_tid_to_fd.end()) {
		assert([&]()->bool {
				std::cerr << "error in set_table_id().\n Already exist.";
				return false;
				}());
	}

	this->map_tid_to_fd[ret_table_id] = fd;

	return ret_table_id;
}

/** Return the table id matching with the given file descriptor. */
int64_t DiskManager::get_table_id(int fd) {
	int64_t ret_table_id;

	// Find table id matching with a given file descriptor
	for (auto& x: this->map_tid_to_fd) {
		if (x.second == fd) {
			ret_table_id = x.first;
			goto func_exit;
		}
	}
	// Set new table id
	ret_table_id = this->set_table_id(fd);

func_exit:
	return ret_table_id;
}

/** Return the file descriptor matching with the given table id. */
int DiskManager::get_fd(int64_t table_id) {
	if (this->map_tid_to_fd.find(table_id) == this->map_tid_to_fd.end()) {
		assert([&]()->bool {
				std::cerr << "erro in get_id().\n Cannot find fd.";
				return false;
				}());
	}

	return this->map_tid_to_fd[table_id];
}

/** Close all file descriptors. */
void DiskManager::close_table_files() {
	for (auto &x : this->map_tid_to_fd) {
		assert(x.second >= 0);
		close(x.second);
	}
}

/** static function def */
static void init_header_page(HeaderPage* header_page) {
	SET_HEADER_FREE_PAGE_NO(header_page, 1);
	SET_HEADER_NUM_OF_PAGES(header_page, DEFAULT_NUM_OF_PAGES);
	SET_HEADER_ROOT_PAGE_NO(header_page, 0);
}

/** Read a page from the disk physically. */
static void file_read_page_internal(int fd, off_t off, void* dest) {
	lseek(fd, off, SEEK_SET);
#ifdef NDEBUG
	if (read(fd, dest, PG_SIZE) != PG_SIZE) {
		MSG("page read error.");
		exit(0);
	}
#else
	assert(PG_SIZE == read(fd, dest, PG_SIZE));
#endif
}

/** Write a page from the disk physically. */
static void file_write_page_internal(int fd, off_t off, const void* src) {
	lseek(fd, off, SEEK_SET);
#ifdef NDEBUG
	if (write(fd, src, PG_SIZE) != PG_SIZE) {
		MSG("page write error.");
		exit(0);
	}
	if (fsync(fd) != 0) {
		MSG("fsync error.");
		exit(0);
	}
#else
	assert(PG_SIZE == write(fd, src, PG_SIZE));
	assert(fsync(fd) == 0);
#endif
}

/** Expand the database. */
static pagenum_t file_expand_file(int fd, HeaderPage* header_page) {
	MSG("file_expand_file().\n");
	pagenum_t ret_page_no;
	FreePage free_page;
	uint64_t num_of_pages;

	/** It should be guaranteed that there is no free page. */
	num_of_pages = GET_HEADER_FREE_PAGE_NO(header_page);
	assert(num_of_pages == 0);

	num_of_pages = GET_HEADER_NUM_OF_PAGES(header_page);
	memset(&free_page, 0x00, PG_SIZE);

	/** Doubling */
	for (uint64_t i = num_of_pages; i < num_of_pages * 2; ++i) {
		if (i == num_of_pages * 2 - 1)
			SET_FREE_NEXT_PAGE_NO(&free_page, 0);
		else
			SET_FREE_NEXT_PAGE_NO(&free_page, i + 1);
		
		SET_PAGE_NO(&free_page, i);
		file_write_page_internal(
				fd, i * PG_SIZE, reinterpret_cast<Page*>(&free_page));
	}

	ret_page_no = num_of_pages;

	SET_HEADER_FREE_PAGE_NO(header_page, num_of_pages + 1);
	SET_HEADER_NUM_OF_PAGES(header_page, num_of_pages * 2);

	file_write_page_internal(fd, 0, reinterpret_cast<Page*>(header_page));

	return ret_page_no;
}

/** Disk Manager APIs */
int open_disk_manager(){
	if (disk_manager != nullptr)
		return -1;
	disk_manager = std::make_unique<DiskManager>();
	if (disk_manager == nullptr)
		return -1;
	return 0;
}

int close_disk_manager() {
	if (disk_manager == nullptr)
		return -1;
	disk_manager->close_table_files();
	disk_manager = nullptr;
	return 0;
}

/** Open the table file with the given pathname. */
int64_t file_open_table_file(const char* pathname) {

	int fd = open(pathname, O_RDWR);
	int64_t ret_table_id;
	if (fd < 0) {
		// Create new file with a default size
		fd = open(pathname, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
		// # Pages=2560 : Header Page (1) + Free Page (2559) 
		if (fd < 0)
			return -1;

		HeaderPage header_page;
		FreePage free_page;
		memset(&header_page, 0x00, sizeof(HeaderPage));
		memset(&free_page, 0x00, sizeof(FreePage));

		init_header_page(&header_page);
		file_write_page_internal(fd, 0, reinterpret_cast<Page*>(&header_page));

		for (int i = 1; i < DEFAULT_NUM_OF_PAGES; ++i) {
			if (i == DEFAULT_NUM_OF_PAGES - 1)
				SET_FREE_NEXT_PAGE_NO(&free_page, 0);
			else
				SET_FREE_NEXT_PAGE_NO(&free_page, i + 1);
			file_write_page_internal(
					fd, i * PG_SIZE, reinterpret_cast<Page*>(&free_page));
		}
	} 

	ret_table_id = disk_manager->get_table_id(fd);
	assert(ret_table_id >= 1);
	return ret_table_id;
}

/*
 * file_alloc_page()
 * @param[in]		table_id : table id returned from open table
 * @return : free page number
 */
pagenum_t file_alloc_page(int64_t table_id) {
	pagenum_t ret_page_no;
	HeaderPage header_page;
	FreePage free_page;
	int fd;
	
	fd = disk_manager->get_fd(table_id);
	file_read_page_internal(fd, 0, reinterpret_cast<Page*>(&header_page));

	if (GET_HEADER_FREE_PAGE_NO(&header_page) == 0) {
		/** No free page. Expand current table file. */
		ret_page_no = file_expand_file(fd, &header_page);
	} else {
		/** Return free page from the linked-list. */
		ret_page_no = GET_HEADER_FREE_PAGE_NO(&header_page);

		// Read Free page and modify free page no in the header page.
		file_read_page_internal(fd, ret_page_no * PG_SIZE, 
				reinterpret_cast<Page*>(&free_page));
		SET_HEADER_FREE_PAGE_NO(&header_page, GET_FREE_NEXT_PAGE_NO(&free_page));

		file_write_page_internal(fd, 0, reinterpret_cast<Page*>(&header_page));
	}

	assert(ret_page_no != 0);
	return ret_page_no;
}

/*
 * file_free_page()
 * @param[in]		table_id	: table id returned from open table
 * @param[in]		pagenum		: page number to be freed.
 * return : void.
 */
void file_free_page(int64_t table_id, pagenum_t pagenum) {
	int fd = disk_manager->get_fd(table_id);

	HeaderPage header_page;
	FreePage free_page;

	file_read_page_internal(fd, 0, 
			reinterpret_cast<Page*>(&header_page));
	file_read_page_internal(fd, pagenum * PG_SIZE, 
			reinterpret_cast<Page*>(&free_page));

	SET_FREE_NEXT_PAGE_NO(&free_page, GET_HEADER_FREE_PAGE_NO(&header_page));
	SET_HEADER_FREE_PAGE_NO(&header_page, pagenum);

	file_write_page_internal(fd, 0, 
			reinterpret_cast<Page*>(&header_page));
	file_write_page_internal(fd, pagenum * PG_SIZE, 
			reinterpret_cast<Page*>(&free_page));
}

/*
 * file_read_page()
 * @param[in]				table_id	: table id returned from open table
 * @param[in]				pagenum		: page number to be read
 * @param[in/out]		dest			: container for in-memory page
 * return : void
 */
void file_read_page(int64_t table_id, pagenum_t pagenum, Page* dest) {
	int fd = disk_manager->get_fd(table_id);
	off_t off = pagenum * PG_SIZE;
	file_read_page_internal(fd, off, dest);

	/** Set page number in in-memory structure. */
	SET_PAGE_NO(dest, pagenum);
}

/*
 * file_read_page()
 * @param[in]			table_id	: table id returned from open table
 * @param[in]			pagenum		: page number to be written
 * @param[in]			src				: in-memory page to be written
 * return : void
 */
void file_write_page(int64_t table_id, pagenum_t pagenum, const Page* src) {
	assert(pagenum == GET_PAGE_NO(src));

	int fd = disk_manager->get_fd(table_id);
	off_t off = pagenum * PG_SIZE;
	file_write_page_internal(fd, off, src);
}

// Close the table files
void file_close_table_files() {
	disk_manager->close_table_files();
}
