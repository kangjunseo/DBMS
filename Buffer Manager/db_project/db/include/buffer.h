#ifndef DB_BUFFER_H_
#define DB_BUFFER_H_

#include "page.h"
#include <memory>

struct ControlBlock {
    int64_t table_id;
    pagenum_t page_num;
    uint32_t is_dirty;
    uint32_t is_pinned;
    int next_idx;
    int prev_idx;
    void * frame;
};

int init_buffer(int num_buf);
pagenum_t buffer_alloc_page(int64_t table_id);
void buffer_free_page(int64_t table_id, pagenum_t pagenum);
void buffer_read_page(int64_t table_id, pagenum_t pagenum, const std::shared_ptr<Page>& dest);
void buffer_write_page(int64_t table_id, pagenum_t pagenum, const Page* src);
int shutdown_buffer();

void control_read_LRU(int buf_index);
void control_new_LRU(int buf_index);

#endif /* DB_BUFFER_H*/
