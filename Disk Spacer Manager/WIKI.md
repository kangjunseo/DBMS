# Project2 - MileStone1  &nbsp;&nbsp; `DiskSpaceManager`

This wiki will provide the overall descriptions about my Disk Space Manager implementation, and also the unittests.

## Overall Descriptions

### file.h

``` Cpp
#define PAGE_SIZE 4096
#define FILE_SIZE 10*1024*1024
```

PAGE_SIZE and FILE_SIZE will be used in implementing Disk Space manager, so I predefined it.
<br/>

``` Cpp
typedef uint64_t pagenum_t;
struct page_t {
    // in-memory page structure
    pagenum_t next_page_num;
    char reserved[PAGE_SIZE-sizeof(next_page_num)];
};

struct header_page_t {
    // in-memory page structure
    pagenum_t first_page_num;
    pagenum_t pages;
    char reserved[PAGE_SIZE-sizeof(first_page_num)-sizeof(pages)];
};
```
Struct `page_t` is used for implementing free page.  
`next_page_num` will provide the next free page number.  
`reserved` is for making the page size as same as 4KB. 
<br/>

I implemented extra struct `header_page_t` to control header page more easily.  
`first_page_num` will provide the page number of the first free page.  
`pages` will provide the number of the all pages(header + free + allocated).

<br/>

``` Cpp
header_page_t * header; //global variance of header

int db_file; //global variance of db_file
int current_file_size;
```

These are some global variances to make my implement more easier.  
<br/>
`header` is the pointer of the opened file's header page.  
Header page will used for almost every functions I implemented, so it is reasonable to set as global.  
<br/>
`db_file` is the file descriptor of the database file that we will open.  
This one is used to close the opened db file in this implementation.  
<br/>
`current_file_size` is used for easier tracking of the currently opened file size.  
It is used when allocating the double size of current db, when the file is full.
<br/> 
<br/>

### 1.&nbsp; `int file_open_database_file(const char* pathname)`

This function opens the database file, but if file don't exists, it will make a new 10MB default database file.  
``` Cpp
db_file = open(pathname,O_SYNC|O_RDWR);   //Open the file from the path
```
Using `open()` I opened the db file and the parameter `O_SYNC` makes all the
writes become synchronized.  
```Cpp
db_file = open(pathname,O_SYNC|O_RDWR|O_CREAT);  //Create new file
```
If there is no file in the path, call `open()` again, but this time, parameter `O_CREAT` is added, so the new file will be created.  
```Cpp
if(ftruncate(db_file,FILE_SIZE)==-1){   //Set file size as 10MB
            fprintf(stderr,"Setting file size error: %s\n", strerror(errno));
            exit(1);
        }
```
Then using `ftruncate()`, set the file size as 10MB.  
<br/>
Other works are initiating new db file(create header, paginate the file, ...).
<br/>
<br/>

### 2.&nbsp; `pagenum_t file_alloc_page(int fd)`

This function allocates an on-disk page from the free page list which is included in header.  
<br/>
When `first_page_num` in `header` is not `0`, it means there are left pages in free page list.  
So in this case, function just returns the new allocated page number, and update the header.  
<br/>
But if `first_page_num` in `header` is `0`, it means the db file is fully allocated, so we need to make the file bigger.  
``` Cpp
if(ftruncate(db_file,2*current_file_size)==-1){   // double allocate the file
            fprintf(stderr,"Doubling file size error: %s\n", strerror(errno));
            exit(1);
        }
```
By following the "File Allocation Rule", I doubled the current file space when there is no pages to allocate.
Then, as we paginated at the default new file, I paginated the increased space.  
<br/>
After this, function returns the new allocated page number, just as normal case.

### 3.&nbsp; `void file_free_page(int fd, pagenum_t pagenum)`


## Unit tests by `Google test`
