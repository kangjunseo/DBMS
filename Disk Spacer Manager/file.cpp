#include "file.h"
#include <fstream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#define FILE_SIZE 10*1024*1024
using namespace std;

// Open existing database file or create one if not existed.
int file_open_database_file(const char* pathname){
    int64_t db_file = open(pathname,O_SYNC|O_RDWR);   //Open the file from the path
    if(db_file==-1) { //no file
        db_file = open(pathname,O_SYNC|O_RDWR|O_CREAT);  //Create new file
        if(db_file==-1){
          fprintf(stderr,"Creating file error: %s\n",strerror(errno));
          return -1;
        }
        if(truncate(pathname,FILE_SIZE)==-1){   //Set file size as 10MB
            fprintf(stderr,"Setting file size error: %s\n", strerror(errno));
            return -1;
        }
        header_page_t * header = new header_page_t; //Create header page
        header->first_page_num=1;
        memset(header,0,sizeof(header_page_t));
        header->pages=(FILE_SIZE-sizeof(header_page_t))/sizeof(page_t);
        //write header at first page
        pwrite(db_file,header,sizeof(header_page_t),0);
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
  //close(db_file);
}
