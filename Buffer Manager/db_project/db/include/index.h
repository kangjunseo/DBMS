#ifndef DB_INDEX_H_
#define DB_INDEX_H_

#include "page.h"

#include <string>
#include <unordered_map>
#include <memory>

class IndexManager {
	private:
		class Table {
			private:
				friend class IndexManager;

				std::string table_name;
				int64_t table_id;

				/** temp statistics */
				int64_t num_recs;
				int64_t num_finds;
				int64_t num_dels;

				void inc_num_recs() { num_recs++; }
				void inc_num_finds() { num_finds++; }
				void inc_num_dels() { num_dels++; }

			public:
				explicit Table(const char* pathname, int64_t tid):
					table_name(pathname), table_id(tid), num_recs(0),
					num_finds(0), num_dels(0) {};

				~Table(){};
		};

	private:
		std::unordered_map<int64_t, std::unique_ptr<Table>> map_tid_to_tab;

		void upd_tab_info(int64_t table_id, const char* table_name);
			
	public:
		int64_t open_table(const char* pathname);
		int find_rec(int64_t tid, int64_t key, char* val, uint16_t* size);
		int insert_rec(int64_t tid, int64_t key, char* ret_val, uint16_t size);
		int delete_rec(int64_t tid, int64_t key);

	private:
		IndexManager(const IndexManager &);
		IndexManager &operator=(const IndexManager &);

	public:
		IndexManager();
		~IndexManager();
		IndexManager(IndexManager&&)=delete;
};


/** Index Manager APIs */

int open_index_manager();
int close_index_manager();

int64_t open_table_file(const char* pathname);

int db_insert_record(int64_t table_id, 
		int64_t key,
		char* value, 
		uint16_t val_size);

int db_find_record(int64_t table_id, 
		int64_t key,
		char* ret_val, 
		uint16_t* val_size);

int db_delete_record(int64_t table_id, 
		int64_t key);


#endif /* DB_INDEX_H */
