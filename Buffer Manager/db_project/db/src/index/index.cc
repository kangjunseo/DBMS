#include "index.h"
#include "file.h"

#include "bpt.h"

#include <cassert>
#include <memory>

static std::unique_ptr<IndexManager> index_manager {nullptr};

/** IndexManager Class APIs */
IndexManager::IndexManager() {
	this->map_tid_to_tab.clear();
}

IndexManager::~IndexManager() {
	this->map_tid_to_tab.clear();
}

void IndexManager::upd_tab_info(int64_t table_id, const char* table_name) {
	if (this->map_tid_to_tab.find(table_id) == this->map_tid_to_tab.end()) {
		this->map_tid_to_tab[table_id] = 
			std::make_unique<Table>(table_name, table_id);
	}
}

/** Return the table id matching with the given pathname. */
int64_t IndexManager::open_table(const char* pathname) {
	int64_t ret;
	std::string given_table_name(pathname);

	/** If the table is already opened, return immediately. */
	for (auto& x: this->map_tid_to_tab) {
		if (x.second->table_name == given_table_name) {
			ret = x.first;
			goto func_exit;
		}
	}

	/** Open table internally. */
	if ((ret = file_open_table_file(pathname)) > 0) {
		index_manager->upd_tab_info(ret, pathname);
	}

func_exit:
	return ret;
}

int IndexManager::find_rec(int64_t tid, int64_t key, 
		char* ret_val, uint16_t* size) {
	int ret;

	if(!(ret = find_record(tid, key, ret_val, size))) {
		this->map_tid_to_tab[tid]->inc_num_finds();
	}

	return ret;
}

int IndexManager::insert_rec(int64_t tid, int64_t key,
		char* val, uint16_t size) {
	int ret;

	if(!(ret = insert_record(tid, key, val, size))) {
		this->map_tid_to_tab[tid]->inc_num_recs();
	}

	return ret;
}

int IndexManager::delete_rec(int64_t tid, int64_t key) {
	int ret;

	if(!(ret = delete_record(tid, key))) {
		this->map_tid_to_tab[tid]->inc_num_dels();
	}

	return ret;
}


/** Index Manager APIs */
int open_index_manager() {
	if (index_manager != nullptr)
		return -1;
	index_manager = std::make_unique<IndexManager>();
	if (index_manager == nullptr)
		return -1;
	return 0;
}

int close_index_manager() {
	if (index_manager == nullptr)
		return -1;
	index_manager = nullptr;
	return 0;
}

int64_t open_table_file(const char* pathname) {
	return index_manager->open_table(pathname);
}

int db_insert_record(int64_t table_id, int64_t key,
		char* value, uint16_t val_size) {

	return index_manager->insert_rec(table_id, key, value, val_size);
}

int db_find_record(int64_t table_id, int64_t key,
		char* ret_val, uint16_t* val_size) {
	return index_manager->find_rec(table_id, key, ret_val, val_size);
}

int db_delete_record(int64_t table_id, int64_t key) {
	return index_manager->delete_rec(table_id, key);
}
