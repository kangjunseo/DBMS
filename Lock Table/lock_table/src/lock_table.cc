#include "lock_table.h"
#include <map>
#include <iostream>
#include <errno.h>
#include <cstdio>

struct lock_t {
    lock_t * prev;
    lock_t * next;
    std::pair<int,int64_t> key;
    pthread_cond_t * cond;
};

typedef struct lock_t lock_t;

std::map<std::pair<int,int64_t>,std::pair<lock_t *,lock_t *>> hash;
pthread_mutex_t lock_table_latch;
pthread_mutex_t mutex;

int init_lock_table() {
    std::map<std::pair<int,int64_t>,int> hash;
    lock_table_latch = PTHREAD_MUTEX_INITIALIZER;
    return 0;
}

lock_t* lock_acquire(int table_id, int64_t key) {
    pthread_mutex_lock(&lock_table_latch);
    std::pair<int,int64_t> hashkey = {table_id, key};
    
    lock_t * new_lock = new lock_t;
    if(new_lock==nullptr){
        std::cout<<"alloc error";
    }
    new_lock->key = hashkey;
    new_lock->next = nullptr;
    
    if(hash.find(hashkey)!=hash.end()){ //not first access
        auto value = hash.at(hashkey);
        
        if(value.second==nullptr){
            new_lock->prev = nullptr;
            new_lock->cond = new pthread_cond_t;
            if(new_lock->cond==nullptr){
                std::cout<<"alloc error";
            }
            *(new_lock->cond) = PTHREAD_COND_INITIALIZER;
        }else{
            new_lock->prev = value.second;
            new_lock->cond = value.second->cond;
            value.second->next = new_lock;
        }
        
        if(value.first!=nullptr){
            hash.at(hashkey) = std::pair<lock_t *,lock_t *>(value.first,new_lock);
            pthread_mutex_lock(&mutex);
            while(new_lock->prev!=nullptr){
                pthread_cond_wait(new_lock->cond,&mutex);
            }
            pthread_mutex_unlock(&mutex);
        }else{
            hash.at(hashkey) = std::pair<lock_t *,lock_t *>(new_lock,new_lock);
        }
        
    }else{  //first access
        pthread_cond_t * new_cond = new pthread_cond_t;
        if(new_cond==nullptr){
            std::cout<<"alloc error";
        }
        *new_cond = PTHREAD_COND_INITIALIZER;
        
        new_lock->prev = nullptr;
        new_lock->cond = new_cond;
        
        hash[hashkey]=std::pair<lock_t *,lock_t *>(new_lock,new_lock);
    }
    
    pthread_mutex_unlock(&lock_table_latch);
    return new_lock;
};

int lock_release(lock_t* lock_obj) {
    pthread_mutex_lock(&lock_table_latch);
    auto prev_value = hash.at(lock_obj->key);
    
    if(lock_obj->next != nullptr){
        lock_obj->next->prev = nullptr;
        pthread_cond_signal(lock_obj->next->cond);
        hash.at(lock_obj->key)=std::pair<lock_t *,lock_t *>(lock_obj->next,prev_value.second);
    }else{
        hash.at(lock_obj->key)=std::pair<lock_t *,lock_t *>(nullptr,nullptr);
    }
    delete lock_obj;
    pthread_mutex_unlock(&lock_table_latch);
    return 0;
}
