#include "buffer.h"
#include <string.h>
#include <stdlib.h>

#define TABLE_NUM 10

extern buffer * buff;
extern tManager * tableM;
extern page_t header;


//buffer
void print_buff(void){
    if(buff->used_cnt == 0){
        printf("Empty buffer\n");
    }
    else{
        
        for(int i =0; i<buff->used_cnt; i++){
            printf("idx : %d, table id : %d, page_num : %llu\n",i,buff->blocks[i].table_id,buff->blocks[i].page_num);
            
        }
    }
    printf("END print_buff\n");
}

void LRU_PUSH(block * p){
    p->LRU_next = buff->LRU_head;
    if(p->LRU_next){
        p->LRU_next->LRU_pre = p;
    }
    p->LRU_pre = NULL;
    buff->LRU_head = p;
    if ( buff->LRU_tail == NULL){
		buff->LRU_tail = p;
	}
}

void LRU_POP(block * p){
    if(p == buff->LRU_tail){
        buff->LRU_tail  = p->LRU_pre;
        if(p->LRU_pre){
            p->LRU_pre->LRU_next = NULL;
        }
    }
    else if(p == buff->LRU_head){
        buff->LRU_head  = p->LRU_next;
        if(p->LRU_next){
            p->LRU_next->LRU_pre = NULL;
        }
    }
    else{
        p->LRU_pre->LRU_next = p->LRU_next;
        p->LRU_next->LRU_pre = p->LRU_pre;
    }
    p->LRU_pre = NULL;
    p->LRU_next = NULL;
}

int init_buff(int num_buf){
    int ret = -1;
    buff = (buffer *)malloc(sizeof(buffer));
    buff->blocks = (block *) malloc(sizeof(block) * num_buf);
    buff->LRU_head = NULL;
    buff->LRU_tail = NULL;
    buff->capacity = num_buf;
    buff->used_cnt = 0;
    
    if(buff == NULL || buff->blocks == NULL){
        return -1;
    }

    for(int i=0; i<num_buf; i++){
        memset(&buff->blocks[i].frame, 0, PAGE_SIZE);
        buff->blocks[i].table_id = -1;
        buff->blocks[i].page_num = 0;
        buff->blocks[i].is_dirty = 0;
        buff->blocks[i].is_pinned = 0;
        buff->blocks[i].LRU_next = NULL;
        buff->blocks[i].LRU_pre = NULL;
    }
    return 0;
}

int buff_find_page(int table_id, pagenum_t pagenum){
    for(int i=0; i<buff->used_cnt; i++){
        if(buff->blocks[i].table_id == table_id && buff->blocks[i].page_num  == pagenum ){
            return i;
        }
    }
    return -1;
}

void buff_evict_page(){
    block * p = buff->LRU_tail;
    while(p != NULL){
        if(p->is_pinned != 1){
            break;
        }
        p = p->LRU_pre;
    }
    if(p==NULL){
        exit(1);
    }

    LRU_POP(p);

    if(p->is_dirty == 1){
        page_t tmp_page;
        memset(&tmp_page,0,PAGE_SIZE);
        memcpy(&tmp_page, &p->frame, PAGE_SIZE);
        file_write_page(tableM->tables[p->table_id-1].dis, p->page_num, &tmp_page);
    }
    memset(&p->frame,0,PAGE_SIZE);
    p->table_id = -1;
    p->page_num = 0;
    p->is_dirty = 0;
    p->is_pinned = 0;
    p->LRU_next = NULL;
    p->LRU_pre = NULL;

    buffer_compact_page();
    buff->used_cnt -= 1;
}

int buff_load_page(int table_id, pagenum_t pagenum){
    if(buff->capacity == buff->used_cnt){
        buff_evict_page();
    }

    int i;
    for(i=0; i<buff->capacity; i++){
        if(buff->blocks[i].table_id == -1){
            break;
        }
    }

    page_t tmp_page;
    memset(&tmp_page,0,PAGE_SIZE);


    file_read_page(tableM->tables[table_id-1].dis, pagenum, &tmp_page);
    memcpy(&buff->blocks[i].frame, &tmp_page,PAGE_SIZE);
    buff->blocks[i].table_id = table_id;
    buff->blocks[i].page_num = pagenum;
    buff->blocks[i].is_dirty = 0;
    buff->blocks[i].is_pinned = 0;
    buff->blocks[i].LRU_next = NULL;
    buff->blocks[i].LRU_pre = NULL;

    LRU_PUSH(&buff->blocks[i]);
    
    buff->used_cnt += 1;

    return i;

}

void buff_pin(int * p){
    *p = 1;
}
void buff_unpin(int * p){
    *p = 0;
}

void buff_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }
    // buff->blocks[idx].is_pinned = 1;
    buff_pin(&buff->blocks[idx].is_pinned);

    memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);

    // buff->blocks[idx].is_pinned = 0;
    buff_unpin(&buff->blocks[idx].is_pinned);
}

void buff_write_page(int table_id, pagenum_t pagenum, page_t * src){
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }
    // buff->blocks[idx].is_pinned = 1;
    buff_pin(&buff->blocks[idx].is_pinned);

    memcpy(&buff->blocks[idx].frame, src, PAGE_SIZE);
    buff->blocks[idx].is_dirty = 1;
    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);

    buff_unpin(&buff->blocks[idx].is_pinned);
    // buff->blocks[idx].is_pinned = 0;
    
}

pagenum_t buff_alloc_page(int table_id){
    buff_read_page(table_id, 0, &header);
    if (header.H.freePnum == 0){  //free page??? ?????? ??????
        file_expand_page(table_id, tableM->tables[table_id-1].dis);
    }
    pagenum_t freePagenum = header.H.freePnum;

    page_t tmp_free;  //????????? free page??? ????????????
    buff_read_page(table_id,freePagenum,&tmp_free);

    header.H.freePnum = tmp_free.F.nfreePnum; //header page??? ???????????? free page ?????? ???????????????
    buff_write_page(table_id,0,&header); //header page??? ???????????? ??????

    memset(&tmp_free,0,PAGE_SIZE);  //????????? free page??? ????????? ?????? ????????? ?????????
    buff_write_page(table_id,freePagenum,&tmp_free);


    return freePagenum;
}

void buff_free_page(int table_id, pagenum_t pagenum){
    buff_read_page(table_id, 0, &header);
    page_t tmp_page;
    buff_read_page(table_id,pagenum,&tmp_page); //free page ?????? page ????????????
    memset(&tmp_page,0,PAGE_SIZE); //?????? 0?????? ?????????
    
    tmp_page.F.nfreePnum = header.H.freePnum; //????????? free page??? free page list??? ??? ?????? ????????????
    header.H.freePnum = pagenum; //header page??? ?????? ???????????? free page??? ???????????? ?????????

    buff_write_page(table_id,pagenum,&tmp_page); //free page??? header page??? update??? ?????? ????????????
    buff_write_page(table_id,0,&header);
}

int buff_clean(void){
    int ret = 0;
    for(int i = 0; i < buff->used_cnt; i++){
        if(buff->blocks[i].is_pinned == 1){
            return -1;
            //pin ?????? ?????? ?????? ????????? ??? ??????
        }
        if(buff->blocks[i].is_dirty == 1){
            page_t tmp_page;
            memset(&tmp_page,0,PAGE_SIZE);
            memcpy(&tmp_page, &buff->blocks[i].frame, PAGE_SIZE);
            file_write_page(tableM->tables[buff->blocks[i].table_id-1].dis, buff->blocks[i].page_num, &tmp_page);
        }
        memset(&buff->blocks[i].frame,0,PAGE_SIZE);
        buff->blocks[i].table_id = -1;
        buff->blocks[i].page_num = 0;
        buff->blocks[i].is_dirty = 0;
        buff->blocks[i].is_pinned = 0;
        buff->blocks[i].LRU_next = NULL;
        buff->blocks[i].LRU_pre = NULL;
    }
    buff->capacity = 0;
    buff->LRU_head = NULL;
    buff->LRU_tail = NULL;
    buff->used_cnt = 0;

    for( int i=0; i<TABLE_NUM; i++){
        if(tableM->tables[i].dis != -1){
            ret = file_close_table(tableM->tables[i].dis);
            tableM->tables[i].dis = -1;
        }
    }

    return ret;
}

int buff_flush_table(int table_id){
    
    for(int i = 0; i < buff->used_cnt; i++){
        if(buff->blocks[i].table_id == table_id ) {
            
            if( buff->blocks[i].is_pinned == 1){
            return -1;
            //pin ?????? ?????? ?????? ????????? ??? ??????
            }
            if(buff->blocks[i].is_dirty == 1){
                page_t tmp_page;
                memset(&tmp_page,0,PAGE_SIZE);
                memcpy(&tmp_page, &buff->blocks[i].frame, PAGE_SIZE);
                file_write_page(tableM->tables[buff->blocks[i].table_id-1].dis, buff->blocks[i].page_num, &tmp_page);
            }
            memset(&buff->blocks[i].frame,0,PAGE_SIZE);
            buff->blocks[i].table_id = -1;
            buff->blocks[i].page_num = 0;
            buff->blocks[i].is_dirty = 0;
            buff->blocks[i].is_pinned = 0;
            buff->blocks[i].LRU_next = NULL;
            buff->blocks[i].LRU_pre = NULL;
            buffer_compact_page();
            buff->used_cnt -= 1;
            i = 0;
            
        }
    }
    
    return 0;
}

void buffer_compact_page(){
    int i,j,k;
    for(i=0; i<buff->used_cnt-1; i++){
        if(buff->blocks[i].table_id == -1){
            for(j=i; j<buff->used_cnt-1;j++){
                if(buff->LRU_head == &buff->blocks[j+1]){
                    buff->LRU_head = &buff->blocks[j];
                }
                if(buff->LRU_tail == &buff->blocks[j+1]){
                    buff->LRU_tail = &buff->blocks[j];
                }
                for(k=0; k<buff->used_cnt; k++){
                    if(k == i){
                        continue;
                    }
                    else if(buff->blocks[k].LRU_next == &buff->blocks[j+1] ){
                        buff->blocks[k].LRU_next = &buff->blocks[j];
                    }
                    else if(buff->blocks[k].LRU_pre == &buff->blocks[j+1] ){
                        buff->blocks[k].LRU_pre = &buff->blocks[j];
                    }
                }
            
            }
        }
    }

    for(i=0; i<buff->used_cnt-1; i++){
        if(buff->blocks[i].table_id == -1){
            for(j=i; j<buff->used_cnt-1;j++){
                memcpy(&buff->blocks[j].frame, &buff->blocks[j+1].frame, PAGE_SIZE);
                buff->blocks[j].table_id = buff->blocks[j+1].table_id;
                buff->blocks[j].page_num = buff->blocks[j+1].page_num;
                buff->blocks[j].is_dirty = buff->blocks[j+1].is_dirty;
                buff->blocks[j].is_pinned = buff->blocks[j+1].is_pinned;
                buff->blocks[j].LRU_next = buff->blocks[j+1].LRU_next;
                buff->blocks[j].LRU_pre = buff->blocks[j+1].LRU_pre;
            
            }
        }
    }
    
    memset(&buff->blocks[buff->used_cnt-1].frame, 0, PAGE_SIZE);
    buff->blocks[buff->used_cnt-1].table_id = -1;
    buff->blocks[buff->used_cnt-1].page_num = 0;
    buff->blocks[buff->used_cnt-1].is_dirty = 0;
    buff->blocks[buff->used_cnt-1].is_pinned = 0;
    buff->blocks[buff->used_cnt-1].LRU_next = NULL;
    buff->blocks[buff->used_cnt-1].LRU_pre = NULL;
    
}



//table
void init_tM(void){
    tableM = (tManager*)malloc(sizeof(tManager));
    for(int i =0; i<10; i++){
        strcpy(tableM->tables[i].name, "\0");
        tableM->tables[i].id = i+1;
        tableM->tables[i].dis = -1;
    }
    
}

int find_table_id(char * pathname){
    int ret = -1;
    for(int i=0; i<TABLE_NUM; i++){

        if(strcmp(tableM->tables[i].name, pathname) == 0){
            ret = file_open_table(tableM->tables[i].id,&tableM->tables[i].dis, pathname);
            if(ret == -1){
                return -1;
            }
            return tableM->tables[i].id;
        }

    }

    return -1;
}

int alloc_new_id(char * pathname){
    int ret = -1;
    for(int i=0; i<TABLE_NUM; i++){
        if(tableM->tables[i].dis == -1 && (strcmp(tableM->tables[i].name,"\0")==0) ){
            memcpy(tableM->tables[i].name, pathname, 20);
            printf("IN alloc_new_id, tableM name %s\n",tableM->tables[i].name);
            ret = file_open_table(tableM->tables[i].id, &tableM->tables[i].dis, pathname);
            return tableM->tables[i].id;
        }
    }
    return -1;
}

int open_table_sec(char * pathname){
    if(tableM == NULL){
        init_tM();
    }

    int ret = -1;
    ret = find_table_id(pathname); //???????????? -1
    if(ret == -1){
        ret = alloc_new_id(pathname); //?????? ???????????? -1
    }

    return ret;
}

int close_table_sec(int table_id){
    int ret = -1;
    ret = buff_flush_table(table_id);
    if(ret == -1){
        return ret;
    }
    for( int i=0; i<TABLE_NUM; i++){
        if(tableM->tables[i].id == table_id){
            ret = file_close_table(tableM->tables[i].dis);
            tableM->tables[i].dis = -1;
            return ret;
        }
    }
    ret = -1;
    return ret;
}