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

int log_file_open(int * file, char * pathname, log_record_t * last_record){
    *file = open(pathname,O_RDWR|O_SYNC);
    

    if(*file<0){  //처음 log_file을 생성하는 경우
        *file = open(pathname, O_CREAT|O_RDWR|O_SYNC, S_IRWXU);
        if(*file<0){
            printf("Creating log_file is failed\n");
            return -1;
        }
        return 0;
    }
    
    //이미 있는 log file을 이용하는 경우
    log_record_t tmp;
    off_t offset = 0;
    int file_size = lseek(*file, 0, SEEK_END);
    // int file_size = lseek(file, 2, SEEK_SET);
    memset(&tmp, 0, LOG_COMR_SIZE);

    // printf("IN log_file_open, fileN : %s, fild : %d, file_size : %d\n",pathname, *file, file_size);
    

    while(offset != file_size){
        lseek(*file, offset, SEEK_SET);
        read(*file, &tmp, LOG_HEADER_SIZE);
        if( tmp.BCR.type  == LOG_BEG_TYPE || tmp.BCR.type  == LOG_COMM_TYPE || tmp.BCR.type  == LOG_ROLL_TYPE){
            offset += LOG_HEADER_SIZE;
        }
        if( tmp.BCR.type == LOG_UPD_TYPE){
            offset += LOG_UPDR_SIZE;
        }
        if( tmp.BCR.type == LOG_COM_TYPE){
            offset += LOG_COMR_SIZE;
        }
        // printf("IN log_file_open, now off_set : %d\n",offset);
        // exit(1);
    }
    memcpy(last_record, &tmp, LOG_COMR_SIZE);

    return 1;
}


void log_file_write_record(int file, int start_point, int type, const log_record_t* src){
    off_t offset = start_point;
    // printf("IN log_file_write_record(in file.c), offset : %ld, start_point : %d\n",offset, start_point);
    lseek(file, offset, SEEK_SET);

    if( type  == LOG_BEG_TYPE || type  == LOG_COMM_TYPE || type  == LOG_ROLL_TYPE){
        write(file, src, LOG_HEADER_SIZE);
    }
    if( type == LOG_UPD_TYPE){
        write(file, src, LOG_UPDR_SIZE);
    }
    if( type == LOG_COM_TYPE){
        write(file, src, LOG_COMR_SIZE);
    }
}

int log_file_read_record(int file, int start_point, int type, const log_record_t* dest){
    off_t offset = start_point;
    int file_size = lseek(file, 0, SEEK_END);

    if(file_size == 0){
        return -2;
    }
    
    lseek(file, offset, SEEK_SET);
    log_record_t tmp;
    read(file, &tmp, LOG_HEADER_SIZE);
    type = tmp.BCR.type;
    // printf("IN log_file_read_record(in file.c), file : %d, offset : %ld, type : %d\n", file, offset, type);
    
    lseek(file, offset, SEEK_SET);

    if( type  == LOG_BEG_TYPE || type  == LOG_COMM_TYPE || type  == LOG_ROLL_TYPE){
        read(file, dest, LOG_HEADER_SIZE);
        off_t now_offset = offset+LOG_HEADER_SIZE;
        if ( now_offset == file_size  ){
            return -1;
        }
        return now_offset;
    }
    if( type == LOG_UPD_TYPE){
        read(file, dest, LOG_UPDR_SIZE);
        off_t now_offset = offset+LOG_UPDR_SIZE;
        if ( now_offset == file_size  ){
            return -1;
        }
        return now_offset;
    }
    if( type == LOG_COM_TYPE){
        read(file, dest, LOG_COMR_SIZE);
        off_t now_offset = offset+LOG_COMR_SIZE;
        if ( now_offset == file_size  ){
            return -1;
        }
        return now_offset;
    }
}