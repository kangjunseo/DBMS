#include "buffer.h"
#include "file.h"
#include "msg.h"

#include <memory>
#include <map>
#include <vector>
#include <cstring>

ControlBlock * buf_CB;
Page * Frames;
std::map<std::pair<int64_t,pagenum_t>,int> hash;
std::vector<int> unused_buf;
int recent_buf_idx, buffer_num;

int init_buffer(int num_buf){
    
    //Allocate the buffer pool with the given number of entries.
    buf_CB = new ControlBlock[num_buf];
    Frames = new Page[num_buf];
    
    //Initialize other fields for your own design.
    for(int i=0; i<num_buf; i++){ unused_buf.push_back(i); }
    buffer_num = num_buf;
    
    return 0;
}

pagenum_t buffer_alloc_page(int64_t table_id){
    MSG("buf alloc.\n");
    pagenum_t ret_page_no;
    std::pair<int64_t,pagenum_t> key = {table_id, 0};
    
    ret_page_no = file_alloc_page(table_id); //allocate page in disk first
    
    if(hash.find(key)!=hash.end()){
        //update if header is on the buffer already
        MSG("header found\n");
        int header_buf_index = hash.find(key)->second;
        HeaderPage head;
        
        memcpy(&head, reinterpret_cast<Page*>(buf_CB[header_buf_index].frame),sizeof(head));
        SET_HEADER_ROOT_PAGE_NO(&head, ret_page_no);
        memcpy(reinterpret_cast<Page*>(buf_CB[header_buf_index].frame),&head,sizeof(head));
        
        buf_CB[header_buf_index].is_dirty=0;
    }
    
    std::pair<int64_t,pagenum_t> key_alloc = {table_id, ret_page_no};
    
    if(hash.find(key_alloc)!=hash.end()){
        //update if allocated page is on the buffer already
        MSG("alloc found\n");
        int alloc_buf_index = hash.find(key_alloc)->second;
        file_read_page(table_id,ret_page_no,&Frames[alloc_buf_index]);
        buf_CB[alloc_buf_index].is_dirty=0;
    }
    
    return ret_page_no;
}

void buffer_free_page(int64_t table_id, pagenum_t pagenum){
    std::pair<int64_t,pagenum_t> key = {table_id, 0};
    file_free_page(table_id,pagenum);
    
    if(hash.find(key)!=hash.end()){
        //update if header is on the buffer already
        int header_buf_index = hash.find(key)->second;
        HeaderPage head;
        
        memcpy(&head,reinterpret_cast<Page*>(buf_CB[header_buf_index].frame),sizeof(head));
        SET_HEADER_FREE_PAGE_NO(&head,pagenum);
        memcpy(reinterpret_cast<Page*>(buf_CB[header_buf_index].frame),&head,sizeof(head));
        
        buf_CB[header_buf_index].is_dirty=0;
    }
}

void control_read_LRU(int buf_index){
    if(recent_buf_idx == buf_index) { return; }
    
    int next = buf_CB[buf_index].next_idx;
    int prev = buf_CB[buf_index].prev_idx;
    
    if(next != -1 && prev != -1){
        buf_CB[next].prev_idx = prev;
    }
    
    buf_CB[prev].next_idx = next;
    buf_CB[buf_index].next_idx = recent_buf_idx;
    buf_CB[buf_index].prev_idx = -1;
    
    recent_buf_idx = buf_index;
}

void control_new_LRU(int buf_index){
    buf_CB[buf_index].next_idx = recent_buf_idx;
    buf_CB[buf_index].prev_idx = -1;
    
    recent_buf_idx = buf_index;
}

void buffer_read_page(int64_t table_id, pagenum_t pagenum, const std::shared_ptr<Page>& dest){
    MSG("buf read",table_id,' ',pagenum,"\n");
    
    std::pair<int64_t,pagenum_t> key = {table_id, pagenum};
    int buf_index;
    
    if(hash.find(key)!=hash.end()){ // already in buffer (hit)
        buf_index=hash.find(key)->second;
        buf_CB[buf_index].is_pinned++;
        MSG("#####hit#####\n");
        control_read_LRU(buf_index);
        
        dest =  std::make_shared<Page>(*reinterpret_cast<Page*>(buf_CB[buf_index].frame));
        /*if(pagenum == 0){
            HeaderPage head;
            memcpy(&head, reinterpret_cast<Page*>(buf_CB[buf_index].frame),sizeof(head));
            MSG("buf read - header root no : ",head.root_page_no,'\n');
        }*/
    }
    else{
        if(unused_buf.size() == 0){
            // choose victim by LRU policy
            int last_idx, victim_idx;
            for(int i = recent_buf_idx; ; i=buf_CB[i].next_idx){
                if(buf_CB[i].next_idx == -1){
                    last_idx = i;
                    break;
                }
            }
            
            for(int i = last_idx; ; i=buf_CB[i].prev_idx){
                if(buf_CB[i].is_pinned > 0){
                    if(i == recent_buf_idx){
                        MSG("Every buffer pool is pinned\n");
                        exit(0);
                    }
                    continue;
                }
                victim_idx = i;
                break;
            }
            
            //evict it
            file_write_page(buf_CB[victim_idx].table_id,
                            buf_CB[victim_idx].page_num,
                            reinterpret_cast<Page*>(buf_CB[victim_idx].frame));
            
            buf_index = victim_idx;
        }
        else{ // use unused buf
            buf_index = unused_buf.front();
            unused_buf.erase(unused_buf.begin());
        }
        hash.insert(make_pair(key,buf_index));
        file_read_page(table_id,pagenum,&Frames[buf_index]);
        buf_CB[buf_index].frame = reinterpret_cast<Page*>(&Frames[buf_index]);
            
        buf_CB[buf_index].table_id = table_id;
        buf_CB[buf_index].page_num = pagenum;
        buf_CB[buf_index].is_dirty = 0;
        buf_CB[buf_index].is_pinned = 1;
        
        control_new_LRU(buf_index);
        
        dest =  std::make_shared<Page>(*reinterpret_cast<Page*>(buf_CB[buf_index].frame));
    }
}

void buffer_write_page(int64_t table_id, pagenum_t pagenum, const Page* src){
    std::pair<int64_t,pagenum_t> key = {table_id, pagenum};
    int buf_index;
    
    if(hash.find(key)==hash.end()){
        MSG("buffer write error : key not found\n");
    }
    
    buf_index=hash.find(key)->second;
    buf_CB[buf_index].frame = const_cast<Page*>(src);
    buf_CB[buf_index].is_dirty = 1;
    buf_CB[buf_index].is_pinned--;
}

int shutdown_buffer(){
    for(int i = 0; i<buffer_num; i++){
        if(buf_CB[i].is_dirty){
            file_write_page(buf_CB[i].table_id,
                            buf_CB[i].page_num,
                            reinterpret_cast<Page*>(buf_CB[i].frame));
        }
    }
    delete[] buf_CB;
    delete[] Frames;
    return 0;
}
