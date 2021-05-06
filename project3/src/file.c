#include "buffer.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

extern page_t header;
#define NEW_PAGES 100

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int file, pagenum_t pagenum, page_t* dest){
    off_t offset = PAGE_SIZE * pagenum;
    lseek(file, offset, SEEK_SET);
    read(file, dest, PAGE_SIZE);
}

// Write an in-memory page(src) to the on-disk page
void file_write_page(int file, pagenum_t pagenum, const page_t* src){
    off_t offset = PAGE_SIZE * pagenum;
    lseek(file, offset, SEEK_SET);
    write(file, src, PAGE_SIZE);
}

//page가 부족하면 100개씩 늘림
void file_expand_page(int table_id, int file){  
    page_t * tmp_page = (page_t *)malloc(sizeof(page_t));
    buff_read_page(table_id, 0, &header);
    
    for(int i = 0; i < NEW_PAGES; i++){
        memset(tmp_page,0,PAGE_SIZE);
        file_write_page(file, header.H.numP+i, tmp_page);
        buff_free_page(table_id, header.H.numP+i);
        // print_buff();
    }
    free(tmp_page);
    header.H.numP += NEW_PAGES;
    buff_write_page(table_id,0,&header);

    
}

//header페에지 초기화
void makeHeaderPage(int table_id, int file){
    memset(&header,0,PAGE_SIZE);
    file_write_page(table_id, file, &header);
    header.H.freePnum = 0;
    header.H.rootPnum = 0;
    header.H.numP = 1;
    // printf("IN makeHeaderPage fd %d\n",file);
    buff_write_page(table_id, 0, &header);
    // printf("IN makeHeaderPage fd %d\n",file);
}

//file을 open함
int file_open_table(int table_id, int * file, char * pathname){
    *file = open(pathname,O_RDWR|O_SYNC);
    if(*file<0){  //처음 database를 생성하는 경우
        *file = open(pathname, O_CREAT|O_RDWR|O_SYNC, S_IRWXU);
        if(*file<0){
            printf("Creating database is failed\n");
            return -1;
        }
        makeHeaderPage(table_id, *file);
    }
    
    buff_read_page(table_id,0,&header); //이미 있는 database를 가져온 경우

    return 0;
}

int file_close_table(int file){
    close(file);
    return 0;
}