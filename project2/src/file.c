#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "file.h"

extern int file;
extern int tableNum;
extern page_t header;

#define NEW_PAGES 100

// file.c or file.cpp
// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page(){
    if (header.H.freePnum == 0){  //free page가 없는 경우
        file_expand_page();
    }
    pagenum_t freePagenum = header.H.freePnum;

    page_t tmp_free;  //첫번째 free page를 읽어오고
    file_read_page(freePagenum,&tmp_free);

    header.H.freePnum = tmp_free.F.nfreePnum; //header page가 가리키는 free page 위치 변경해주고
    file_write_page(0,&header); //header page의 변경사항 저장

    memset(&tmp_free,0,PAGE_SIZE);  //반환될 free page의 내용을 모두 지우고 넘겨줌
    file_write_page(freePagenum,&tmp_free);

    return freePagenum;
}

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum){
    page_t tmp_page;
    file_read_page(pagenum,&tmp_page); //free page 만들 page 읽어오고
    memset(&tmp_page,0,PAGE_SIZE); //전부 0으로 초기화
    
    tmp_page.F.nfreePnum = header.H.freePnum; //새로운 free page를 free page list의 맨 앞에 넣어주고
    header.H.freePnum = pagenum; //header page가 새로 만들어진 free page를 가리키게 해주고

    file_write_page(pagenum,&tmp_page); //free page와 header page의 update한 값을 저장해줌
    file_write_page(0,&header);
}
// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest){
    off_t offset = PAGE_SIZE * pagenum;
    lseek(file, offset, SEEK_SET);
    read(file, dest, PAGE_SIZE);
}
// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src){
    off_t offset = PAGE_SIZE * pagenum;
    lseek(file, offset, SEEK_SET);
    write(file, src, PAGE_SIZE);
}
//page가 부족하면 100개씩 늘림
void file_expand_page(void){  
    page_t * tmp_page = (page_t *)malloc(sizeof(page_t));
    for(int i = 0; i < NEW_PAGES; i++){
        memset(tmp_page,0,PAGE_SIZE);
        file_write_page(header.H.numP+i, tmp_page);
        file_free_page(header.H.numP+i);
    }
    free(tmp_page);
    header.H.numP += NEW_PAGES;
    file_write_page(0,&header);
}
//header페에지 초기화
void makeHeaderPage(void){
    memset(&header,0,PAGE_SIZE);
    header.H.freePnum = 0;
    header.H.rootPnum = 0;
    header.H.numP = 1;
    file_write_page(0, &header);
}
//file을 open함
int file_open_talbe(char * pathname){
    file = open(pathname,O_RDWR|O_SYNC);
    if(file<0){  //처음 database를 생성하는 경우
        file = open(pathname, O_CREAT|O_RDWR|O_SYNC, S_IRWXU);
        if(file<0){
            printf("Creating database is failed\n");
            return -1;
        }
        makeHeaderPage();
    }
    
    file_read_page(0,&header); //이미 있는 database를 가져온 경우

    return 0;
}