#include "file.h"
#include <fstream>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include "sys/stat.h"
using namespace std;

int db_file;
header_page_t * header;

// Open existing database file or create one if not existed.
int file_open_database_file(const char* pathname){
    db_file = open(pathname,O_SYNC|O_RDWR,S_IRUSR|S_IWUSR);   //Open the file from the path

    if(db_file==-1) { //no file
        db_file = open(pathname,O_SYNC|O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);  //Create new file
        if(db_file==-1){
            fprintf(stderr,"Creating file error: %s\n",strerror(errno));
            exit(1);
        }

        if(ftruncate(db_file,FILE_SIZE)==-1){   //Set file size as 10MB
            fprintf(stderr,"Setting file size error: %s\n", strerror(errno));
            exit(1);
        }
        
        header = new header_page_t; //Create header page
        memset(header,0,PAGE_SIZE);
        header->first_page_num=1;
        header->pages=FILE_SIZE/PAGE_SIZE;

        for(int i=1;i<header->pages;i++){  //paginate the file
            free_page_t * free = new free_page_t;
            if(i==header->pages-1){
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
        header = new header_page_t;
        memset(header,0,PAGE_SIZE);
        pread(db_file,header,PAGE_SIZE,0);   //Read the header
    }
    return db_file;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(int fd){
    pread(db_file,header,PAGE_SIZE,0);   //Read the header
    if(header->first_page_num==0){  
        if(ftruncate(db_file,2*PAGE_SIZE*header->pages)==-1){   // double allocate the file
            fprintf(stderr,"Doubling file size error: %s\n", strerror(errno));
            exit(1);
        }
      int previous_pages = header->pages;
      header->pages*=2;
      for(int i=previous_pages;i<header->pages;i++){  //paginate the increased file
          free_page_t * free = new free_page_t;
          if(i==header->pages-1){
              free->next_page_num=0;
          }else{
              free->next_page_num=i+1;
          }
          pwrite(db_file,free,PAGE_SIZE,i*PAGE_SIZE);
          delete free;
        }
    }
    pagenum_t new_page = header->first_page_num;
    pagenum_t * next;
    pread(fd,next,sizeof(pagenum_t),new_page*PAGE_SIZE);
    header->first_page_num=*next;
    pwrite(fd,header,PAGE_SIZE,0);    //Write the header
    delete next;
    return new_page;
}

// Free an on-disk page to the free page list
void file_free_page(int fd, pagenum_t pagenum){
    pread(db_file,header,PAGE_SIZE,0);   //Read the header
    pagenum_t prev = header->first_page_num;

    free_page_t * temp = new free_page_t;
    memset(temp,0,PAGE_SIZE);
    temp->next_page_num=prev;
    header->first_page_num=pagenum;

    pwrite(db_file,temp,PAGE_SIZE,pagenum*PAGE_SIZE); //Free the page and keep it to free page list
    pwrite(fd,header,PAGE_SIZE,0);    //Write the header
    delete temp;
}

// Read an on-disk page into the in-memory page structure(dest) 
void file_read_page(int fd, pagenum_t pagenum, page_t* dest){
    if(pread(fd,dest,PAGE_SIZE,pagenum*PAGE_SIZE)!=PAGE_SIZE){
        fprintf(stderr,"read error\n");
        exit(1);
    }
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int fd, pagenum_t pagenum, const page_t* src){
    if(pwrite(fd,src,PAGE_SIZE,pagenum*PAGE_SIZE)!=PAGE_SIZE){
        fprintf(stderr,"no space to write\n");
        exit(1);
   }
}

// Stop referencing the database file
void file_close_database_file(){
    delete header;
    if(close(db_file)==-1){
        fprintf(stderr,"Closing file error: %s\n",strerror(errno));
        exit(1);
    }else{
        printf("Success fully closed.\n");
    }
    exit(0);
}
