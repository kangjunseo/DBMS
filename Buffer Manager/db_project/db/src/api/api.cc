#include "api.h"
#include "index.h"
#include "file.h"
#include "buffer.h"

int64_t open_table(char* pathname) {
	return open_table_file(pathname);
}

int db_insert(int64_t table_id, int64_t key, 
		char* value, uint16_t val_size) {
	return db_insert_record(table_id, key, value, val_size);
}

int db_find(int64_t table_id, int64_t key, 
		char* ret_val, uint16_t* val_size) {
	return db_find_record(table_id, key, ret_val, val_size);
}

int db_delete(int64_t table_id, int64_t key) {
	return db_delete_record(table_id, key);
}

int init_db(int num_buf) {
	int ret;
	ret = open_index_manager();
	if (ret != 0)
		return -1;
    ret = init_buffer(num_buf);
    if (ret != 0)
        return -1;
	ret = open_disk_manager();
	if (ret != 0)
		return -1;
	return 0;
}

int shutdown_db() {
	int ret;
	ret = close_index_manager();
	if (ret != 0)
		return -1;
    ret = shutdown_buffer();
    if (ret != 0)
        return -1;
	ret = close_disk_manager();
	if (ret != 0)
		return -1;
	return 0;
}
