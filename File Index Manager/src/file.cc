#include "file.h"
#include <fstream>
#include <iostream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "sys/stat.h"
using namespace std;

int64_t db_files[19];
const char * paths[19];
int db_idx = 0;
header_page_t * header=NULL;

// Open existing database file or create one if not existed.
int64_t file_open_database_file(const char* pathname){
    if(db_idx==19){ //check if full
        fprintf(stderr,"Number of table full.\n");
        return -1;
    }
    
    for(int i=0;i<19;i++){ //check uniqueness
        if(paths[i]==pathname){
            fprintf(stderr,"Duplicate path.\n");
            return -1;
        }
    }
    paths[db_idx]=pathname;
    
    db_files[db_idx] = open(pathname,O_SYNC|O_RDWR,S_IRUSR|S_IWUSR);   //Open the file from the path

    if(db_files[db_idx]==-1) { //no file
        db_files[db_idx] = open(pathname,O_SYNC|O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);  //Create new file
        if(db_files[db_idx]==-1){
            fprintf(stderr,"Creating file error: %s\n",strerror(errno));
	        close(db_files[db_idx]);
            exit(1);
        }

        if(ftruncate(db_files[db_idx],FILE_SIZE)==-1){   //Set file size as 10MB
            fprintf(stderr,"Setting file size error: %s\n", strerror(errno));
	        close(db_files[db_idx]);
            exit(1);
        }
        
        header = new header_page_t; //Create header page
        if (header==NULL){
            fprintf(stderr,"Allocation error\n");
            exit(1);
        }
        memset(header,0,PAGE_SIZE);
        header->first_page_num=1;
        header->pages=FILE_SIZE/PAGE_SIZE;
        header->root=0;
        for(int i=1;i<header->pages;i++){  //paginate the file
            free_page_t free;
            if(i==header->pages-1){
                free.next_page_num=0;
            }else{
                free.next_page_num=i+1;
            }
            pwrite(db_files[db_idx],&free,PAGE_SIZE,i*PAGE_SIZE);
            sync();
        }
        //write header at first page
        pwrite(db_files[db_idx],header,PAGE_SIZE,0);
        sync();
    }else{
        header = new header_page_t;
        if (header==NULL){
            fprintf(stderr,"Allocation error\n");
            exit(1);
        }
        memset(header,0,PAGE_SIZE);
        pread(db_files[db_idx],header,PAGE_SIZE,0);   //Read the header
    }
    return db_files[db_idx++];
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int64_t table_id){
    pread(table_id,header,PAGE_SIZE,0);   //Read the header
      if(header->first_page_num==0){  
        if(ftruncate(table_id,2*PAGE_SIZE*header->pages)==-1){   // double allocate the file
            fprintf(stderr,"Doubling file size error: %s\n", strerror(errno));
            exit(1);
        }
      int previous_pages = header->pages;
      header->pages*=2;
      for(int i=previous_pages;i<header->pages;i++){  //paginate the increased file
          free_page_t free;
	  if(i==header->pages-1){
              free.next_page_num=0;
          }else{
              free.next_page_num=i+1;
          }
          pwrite(table_id,&free,PAGE_SIZE,i*PAGE_SIZE);
          sync();
        }
    }
    pagenum_t new_page = header->first_page_num;
    free_page_t next;
    pread(table_id,&next,PAGE_SIZE,new_page*PAGE_SIZE);
    header->first_page_num=next.next_page_num;
    pwrite(table_id,header,PAGE_SIZE,0);    //Write the header
    sync();
    return new_page;
}

// Free an on-disk page to the free page list
void file_free_page(int64_t table_id, pagenum_t pagenum){
    pread(table_id,header,PAGE_SIZE,0);   //Read the header
    pagenum_t prev = header->first_page_num;

    free_page_t temp;
    temp.next_page_num=prev;
    header->first_page_num=pagenum;

    pwrite(table_id,&temp,PAGE_SIZE,pagenum*PAGE_SIZE); //Free the page and keep it to free page list
    sync();
    pwrite(table_id,header,PAGE_SIZE,0);    //Write the header
    sync();
}

// Read an on-disk page into the in-memory page structure(dest) 
void file_read_page(int64_t table_id, pagenum_t pagenum, page_t* dest){
    if(pread(table_id,dest,PAGE_SIZE,pagenum*PAGE_SIZE)!=PAGE_SIZE){
        fprintf(stderr,"read error: %s\n",strerror(errno));
        exit(1);
    }
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int64_t table_id, pagenum_t pagenum, const page_t* src){
    if(pwrite(table_id,src,PAGE_SIZE,pagenum*PAGE_SIZE)!=PAGE_SIZE){
        fprintf(stderr,"no space to write: %s\n",strerror(errno));
        exit(1);
    }
    sync();
}

// Stop referencing the database file
void file_close_database_file(){
    delete header;
    for(int i=0;i<db_idx;i++){
    if(close(db_files[i])==-1){
        fprintf(stderr,"Closing file error: %s\n",strerror(errno));
        exit(1);
     }
    }
    printf("Success fully closed.\n");
}
