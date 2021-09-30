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
    char reserved[PAGE_SIZE];
};
```
Struct `page_t` is the basic implementation of all pages.  

``` Cpp
typedef uint64_t pagenum_t;
struct free_page_t {
    pagenum_t next_page_num;
    char reserved[PAGE_SIZE-sizeof(next_page_num)];
};
```
Struct `free_page_t` is used for implementing free page.  
It will be also used when paginating the newly created or doubly allocated files.    
`next_page_num` will provide the next free page number.  
`reserved` is for making the page size as same as 4KB. 
``` Cpp
struct header_page_t {
    pagenum_t first_page_num;
    pagenum_t pages;
    char reserved[PAGE_SIZE-sizeof(first_page_num)-sizeof(pages)];
};
```
I implemented extra struct `header_page_t` to control header page more easily.  
`first_page_num` will provide the page number of the first free page.  
`pages` will provide the number of the all pages(header + free + allocated).

<br/>

``` Cpp
header_page_t * header; //global variable of header

int db_file; //global variable of db_file
int current_file_size;
```
### Global variables

These are some global variables to make my implement more easier.  
<br/>
`header` is the pointer of the opened file's header page.  
Header page will used for almost every functions I implemented, so it is reasonable to set as global.  
<br/>
`db_file` is the file descriptor of the database file that we will open.  
This one is used to close the opened db file in this implementation.  
<br/> 
Actually, I first defined these variables in 'file.h', but "Multiple definition error" occurs.  
With help of StackOverFlow, I defined these variables in the right place.  
[Related Link](https://stackoverflow.com/questions/17764661/multiple-definition-of-linker-error)
<br/>
<br/>
### 1.&nbsp; `int file_open_database_file(const char* pathname)`

This function opens the database file, but if file don't exists, it will make a new 10MB default database file.

#### Open(or Create) the file  
``` Cpp
db_file = open(pathname,O_SYNC|O_RDWR,S_IRUSR|S_IWUSR);   //Open the file from the path
```
Using `open()` I opened the db file and the parameter `O_SYNC` makes all the
writes become synchronized.   
While executing the function with googletest, there was some permission issue in Ubuntu.  
So, I added two parameters(`S_IRUSR|S_IWUSR`) for the authority.
```Cpp
db_file = open(pathname,O_SYNC|O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);  //Create new file
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
#### Initiate the header
``` Cpp
 header = new header_page_t;
 memset(header,0,PAGE_SIZE);
//write header at first page
 pwrite(db_file,header,PAGE_SIZE,0);
 sync(); // sync after writing
```
`header` is globally declared in "file.cc", but it points `NULL` now.  
Before we go through other function, `header` must be initiated.
```Cpp
 header->first_page_num=1;
 header->pages=FILE_SIZE/PAGE_SIZE;
```
If header is newly created, we nead to set the member variable of the header.  
Then, don't forget to write the initiated header.  
<br/>
If file is not newly created, just read the header from the file.  


#### Paginating new file

```Cpp
 for(int i=1;i<header->pages;i++){  //paginate the file
     free_page_t * free = new free_page_t;
     if(i==header->pages-1){
         free->next_page_num=0;
         }else{
         free->next_page_num=i+1;
     }
     pwrite(db_file,free,PAGE_SIZE,i*PAGE_SIZE);
     sync();
     delete free;
 }
```
If file is newly created, we need to paginate the extra space of files in advance.  
I just linearly paginated the file, and made a implicit free page linked list.

<br/>
<br/>

### 2.&nbsp; `pagenum_t file_alloc_page(int fd)`

This function allocates an on-disk page from the free page list which is included in header.  
<br/>
When `first_page_num` in `header` is not `0`, it means there are left pages in free page list.  
So in this case, function just returns the new allocated page number, and update the header.  
<br/>
But if `first_page_num` in `header` is `0`, it means the db file is fully allocated, so we need to make the file bigger.
#### Doubling the current file space  
``` Cpp
if(header->first_page_num==0){  
  if(ftruncate(db_file,2*PAGE_SIZE*header->pages)==-1){   // double allocate the file
    fprintf(stderr,"Doubling file size error: %s\n", strerror(errno));
    exit(1);
    }
```
By following the "File Allocation Rule", I doubled the current file space when there is no pages to allocate.  
Then, as we paginated at the default new file, I paginated the increased space.  
The paginating code is same as what I implemented in `int file_open_database_file(const char* pathname)`.    
(Check the above - " Paginating new file ")    
After this, function returns the new allocated page number, just as normal case.  
#### Allocating the new file
``` Cpp
pagenum_t new_page = header->first_page_num;
pagenum_t * next;
pread(fd,next,sizeof(pagenum_t),new_page*PAGE_SIZE);
header->first_page_num=*next;
pwrite(fd,header,PAGE_SIZE,0);    //Write the header
sync();
delete next;
return new_page;
```
Allocate the new page using the header, then update the header info.  
Temporally used `pagenum_t * next` is deleted right away.  
<br/>
### 3.&nbsp; `void file_free_page(int fd, pagenum_t pagenum)`

This function is called to allocated page's number.  
This function frees the allocated page and returns that page to free page list.  

``` Cpp
pread(fd,header,PAGE_SIZE,0);   //Read the header
pagenum_t prev = header->first_page_num;

free_page_t * temp = new free_page_t;
temp->next_page_num=prev;
header->first_page_num=pagenum;

pwrite(fd,temp,PAGE_SIZE,pagenum*PAGE_SIZE); //Free the page and keep it to free page list
sync();
pwrite(fd,header,PAGE_SIZE,0);    //Write the header
sync();
delete temp;
```

I used "LIFO" to make the process faster.  
To explain it more, Last Input (recently freed page) goes to First Output (header->first_page_num).  
This prevents the unnecessary iteration of free pages.  
<br/>
### 4.&nbsp; Other APIs (read, write, close)
Other APIs are relatively short compare to APIs above.
#### `void file_read_page(int fd, pagenum_t pagenum, page_t* dest)`
``` Cpp
if(pread(fd,dest,PAGE_SIZE,pagenum*PAGE_SIZE)!=PAGE_SIZE){
    fprintf(stderr,"read error: %s\n",strerror(errno));
    exit(1);
}
```
This function reads the page of `pagenum`, and returns it to `dest`.  
If return value of `pread()` is not `4096`, it prints the error message and shut down the db.  

#### `void file_write_page(int fd, pagenum_t pagenum, const page_t* src)`
``` Cpp
if(pwrite(fd,src,PAGE_SIZE,pagenum*PAGE_SIZE)!=PAGE_SIZE){
    fprintf(stderr,"no space to write: %s\n",strerror(errno));
    exit(1);
}
sync();
```
This function writes the page of `pagenum`, and the input buffer is `src`.  
If return value of `pwrite()` is not `4096`, it prints the error message and shut down the db.

#### `void file_close_database_file()`
``` Cpp
delete header;
if(close(db_file)==-1){
    fprintf(stderr,"Closing file error: %s\n",strerror(errno));
    exit(1);
}else{
    printf("Success fully closed.\n");
}
exit(0);
```
This function frees the header, which used for global variable.  
Then it closes the db file.  
If it closed abnormally, print the error message, if closed successfully, print success message.  
<br/>
<br/>
## Unit tests by `Google test`

In this project, `Google test` is used as unit test framework.  
There will be 3 tests following, which are "FileInitTest", "FileAllocTest", and "FileReadWriteTest".  
(Real name I implemented can be a bit different.). 

### `FileInitTest`

I actually used the basic code which TA give it to us, just a bit change of pathname.  

``` Cpp
  int fd;                                 // file descriptor
  std::string pathname = "file_test.db";  // customize it to your test file

  // Open a database file
  fd = file_open_database_file(pathname.c_str());

  // Check if the file is opened
  ASSERT_TRUE(fd >= 0);  // change the condition to your design's behavior

  // Check the size of the initial file
  int num_pages = /* fetch the number of pages from the header page */ 2560;
  EXPECT_EQ(num_pages, FILE_SIZE / PAGE_SIZE)
      << "The initial number of pages does not match the requirement: "
      << num_pages;

  // Close all database files
  file_close_database_file();

  // Remove the db file
  int is_removed = remove(pathname.c_str());

  ASSERT_EQ(is_removed, 0);
```
It checks the file is opened successfully, the file size is correct, and the file is closed well.  

### `FileAllocTest`

This test calls `file_alloc_page()` twice, and free one of them by `file_free_page()`.
Then it check if allocated page is NOT in the free page list, and check if free page is IN the free page list.  

``` Cpp
  int IsFoundA = 0;
  int IsFoundF = 0;
  header_page_t * header=new header_page_t;
  pread(fd,header,PAGE_SIZE,0);
  pagenum_t iter=header->first_page_num;
  for (int i=0;i<header->pages;i++){ //prevent inf loop
    if(iter == 0){
      break;
    }
    if(freed_page == iter){
      IsFoundF = 1;
    }else if(allocated_page == iter){
      IsFoundA = 1;
    }
    free_page_t * it_page = new free_page_t;
    pread(fd,it_page,PAGE_SIZE,iter*PAGE_SIZE);
    iter = it_page->next_page_num;
    delete it_page;
  }
  EXPECT_EQ(IsFoundF,1);
  EXPECT_EQ(IsFoundA,0);
```
This code is the part of iterating the free page list.  
`IsFoundF` is for free page, and `IsFoundA` is for allocated page.  

### `FileReadWriteTest`

``` Cpp
   page_t * src = new page_t;
   for(int i=0;i<PAGE_SIZE;i++){
     src->reserved[i]='a';
   }
   page_t * dest = new page_t;
   pagenum_t  pagenum = file_alloc_page(fd);
   
   file_write_page(fd, pagenum, src);
   file_read_page(fd, pagenum, dest);

   EXPECT_EQ(memcmp(src,dest,PAGE_SIZE), 0);
```

### Result of Unit Test
<img width="1282" alt="스크린샷 2021-09-30 오후 10 27 37" src="https://user-images.githubusercontent.com/88201041/135468120-e5dc0569-b2c7-4ec5-8d2e-86b3ebf228a0.png">
All of three tests are successed.  

More information about test log is in the picture below.  
To get this log, I used "db_project/build/Testing/Temporary/"LastTest.log"".  
<br/>

<img width="1264" alt="스크린샷 2021-09-30 오후 10 27 46" src="https://user-images.githubusercontent.com/88201041/135468152-72e8dcee-8f9a-4e36-a180-61dfe66080c2.png">
<img width="669" alt="스크린샷 2021-09-30 오후 10 31 59" src="https://user-images.githubusercontent.com/88201041/135468158-81912b3e-a0c2-443e-a624-5b3156d8ff02.png">

