#ifndef _DB_BPT_H_
#define _DB_BPT_H_

#include <stdint.h>

int find_record(int64_t tid, int64_t key,
		char* ret_val, uint16_t* size);

int insert_record(int64_t tid, int64_t key,
		char* val, uint16_t size);

int delete_record(int64_t tid, int64_t key);

#endif /* DB_BPT_H */
