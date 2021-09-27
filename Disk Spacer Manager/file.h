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
int64_t file_open_database_file(char* path){
    int64_t db_file = open(path,O_SYNC|O_RDWR);   //Open the file from the path
    if(db_file==-1) { //no file
        ofstream ofs(path); //Create new file
        if(truncate(path,FILE_SIZE)==-1){   //Set file size as 10MB
            fprintf(stderr,"error: %s\n", strerror(errno));
            return 1;
        }
        ofs.close();
        db_file = open(path,O_SYNC|O_RDWR);
    }
    return db_file;
}

// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
    if()
}
// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
}
// Read an on-disk page into the in-memory page structure(dest) void file_read_page(pagenum_t pagenum, page_t* dest){
}
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
}
// Stop referencing the database file
void file_close_database_file(){
}
