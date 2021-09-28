#include "file.h"
#include <fstream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
using namespace std;

// Open existing database file or create one if not existed.
int file_open_database_file(const char* pathname){
    db_file = open(pathname,O_SYNC|O_RDWR);   //Open the file from the path
    if(db_file==-1) { //no file
        db_file = open(pathname,O_SYNC|O_RDWR|O_CREAT);  //Create new file
        if(db_file==-1){
          fprintf(stderr,"Creating file error: %s\n",strerror(errno));
          return -1;
        }

        if(ftruncate(db_file,FILE_SIZE)==-1){   //Set file size as 10MB
            fprintf(stderr,"Setting file size error: %s\n", strerror(errno));
            return -1;
        }
        
        header = new header_page_t; //Create header page
        memset(header,0,PAGE_SIZE);
        header->first_page_num=1;
        header->pages=(FILE_SIZE-sizeof(header_page_t))/PAGE_SIZE;

        for(int i=1;i<=header->pages;i++){  //paginate the file
          page_t * free = new page_t;
          if(i==header->pages){
            free->next_page_num=0;
          }else{
            free->next_page_num=i+1;
          }
          pwrite(db_file,free,PAGE_SIZE,i*PAGE_SIZE);
          delete free;
        }
        //write header at first page
        pwrite(db_file,header,PAGE_SIZE,0);
    }else{
      pread(db_file,header,PAGE_SIZE,0);   //Read the header
    }
    return db_file;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int fd){
    
}

// Free an on-disk page to the free page list
void file_free_page(int fd, pagenum_t pagenum){
  
}

// Read an on-disk page into the in-memory page structure(dest) 
void file_read_page(int fd, pagenum_t pagenum, page_t* dest){

}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int fd, pagenum_t pagenum, const page_t* src){
  if(pwrite(fd,src,sizeof(page_t),pagenum*sizeof(page_t))!=sizeof(page_t))
    fprintf(stderr,"no space to write");
}

// Stop referencing the database file
void file_close_database_file(){
  //free
  close(db_file);
}
