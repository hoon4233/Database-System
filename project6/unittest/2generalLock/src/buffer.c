#include "buffer.h"
#include <string.h>
#include <stdlib.h>

#define TABLE_NUM 10

extern buffer * buff;
extern tManager * tableM;
extern page_t header;

pthread_mutex_t buff_latch = PTHREAD_MUTEX_INITIALIZER;

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
        // pthread_mutex_lock(&p->page_latch);
        if(p->is_pinned == 0){
            break;
        }
        p = p->LRU_pre;
        if(p==NULL){
            p = buff->LRU_tail;
        }
        // pthread_mutex_unlock(&p->page_latch);
    }
    // if(p==NULL){
    //     exit(1);
    // }

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
    // pthread_mutex_unlock(&p->page_latch);

    buffer_compact_page();
    buff->used_cnt -= 1;
}

int buff_load_page(int table_id, pagenum_t pagenum){
    if(buff->capacity == buff->used_cnt){
        int tmp_id = pthread_self();
        // printf("before evict id : %d\n", tmp_id);
        // print_buff();
        buff_evict_page();
        // printf("after evict\n");
        // print_buff();
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
    pthread_mutex_init(&buff->blocks[i].page_latch, NULL);

    LRU_PUSH(&buff->blocks[i]);
    
    buff->used_cnt += 1;
    // printf("after evict, pagenum : %d\n",pagenum);
    // print_buff();

    return i;

}

void buff_pin(int * p){
    *p += 1;
}
void buff_unpin(int * p){
    *p -= 1;
}

void buff_get_page_latch(int table_id, pagenum_t pagenum){
    pthread_mutex_lock(&buff_latch);
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }

    pthread_mutex_lock(&buff->blocks[idx].page_latch);
    buff_pin(&buff->blocks[idx].is_pinned);

    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);

    pthread_mutex_unlock(&buff_latch);
}

void buff_release_page_latch(int table_id, pagenum_t pagenum){
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    printf("IN rel_page_latch, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
    
    buff_unpin(&buff->blocks[idx].is_pinned);
    pthread_mutex_unlock(&buff->blocks[idx].page_latch);
}


void buff_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    pthread_mutex_lock(&buff_latch);
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    // printf("IN buff_read_page, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }

    pthread_mutex_lock(&buff->blocks[idx].page_latch);
    buff_pin(&buff->blocks[idx].is_pinned);

    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);
    memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);

    pthread_mutex_unlock(&buff_latch);


    // memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    // LRU_POP(&buff->blocks[idx]);
    // LRU_PUSH(&buff->blocks[idx]);

    
    buff_unpin(&buff->blocks[idx].is_pinned);
    pthread_mutex_unlock(&buff->blocks[idx].page_latch);
}

void buff_read_page_new(int table_id, pagenum_t pagenum, page_t * dest){
    pthread_mutex_lock(&buff_latch);
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    printf("IN read_page_new, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }

    // printf("C\n");
    pthread_mutex_lock(&buff->blocks[idx].page_latch);
    // printf("D\n");
    buff_pin(&buff->blocks[idx].is_pinned);

    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);
    memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);

    pthread_mutex_unlock(&buff_latch);


    // memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    // LRU_POP(&buff->blocks[idx]);
    // LRU_PUSH(&buff->blocks[idx]);

    
    // buff_unpin(&buff->blocks[idx].is_pinned);
    // pthread_mutex_unlock(&buff->blocks[idx].page_latch);
}

void buff_write_page(int table_id, pagenum_t pagenum, page_t * src){
    pthread_mutex_lock(&buff_latch);
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }

    pthread_mutex_lock(&buff->blocks[idx].page_latch);
    buff_pin(&buff->blocks[idx].is_pinned);

    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);
    memcpy(&buff->blocks[idx].frame, src, PAGE_SIZE);
    buff->blocks[idx].is_dirty = 1;

    pthread_mutex_unlock(&buff_latch);
    

    // memcpy(&buff->blocks[idx].frame, src, PAGE_SIZE);
    // buff->blocks[idx].is_dirty = 1;
    // LRU_POP(&buff->blocks[idx]);
    // LRU_PUSH(&buff->blocks[idx]);

    buff_unpin(&buff->blocks[idx].is_pinned);
    pthread_mutex_unlock(&buff->blocks[idx].page_latch);
    
    
}

void buff_update_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    // pthread_mutex_lock(&buff_latch);
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    printf("IN update_read_page, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }
    // printf("update, read idx : %d\n",idx);
    // print_buff();

    // pthread_mutex_lock(&buff->blocks[idx].page_latch);
    // buff_pin(&buff->blocks[idx].is_pinned);

    memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    // LRU_POP(&buff->blocks[idx]);
    // LRU_PUSH(&buff->blocks[idx]);

    // printf("update, read, dest.numkeys : %d", dest->L.header.numKeys);

    // pthread_mutex_unlock(&buff_latch);


    // memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    // LRU_POP(&buff->blocks[idx]);
    // LRU_PUSH(&buff->blocks[idx]);
}

void buff_update_write_page(int table_id, pagenum_t pagenum, page_t * src){
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    printf("IN update_write_page, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);

    // printf("update, write idx : %d\n",idx);
    // print_buff();
    // printf("update, read, src.numkeys : %d", src->L.header.numKeys);
    
    memcpy(&buff->blocks[idx].frame, src, PAGE_SIZE);
    buff->blocks[idx].is_dirty = 1;
    // pthread_mutex_unlock(&buff_latch);
    // LRU_POP(&buff->blocks[idx]);
    // LRU_PUSH(&buff->blocks[idx]);

    buff_unpin(&buff->blocks[idx].is_pinned);
    pthread_mutex_unlock(&buff->blocks[idx].page_latch);
}

pagenum_t buff_alloc_page(int table_id){
    buff_read_page(table_id, 0, &header);
    if (header.H.freePnum == 0){  //free page가 없는 경우
        file_expand_page(table_id, tableM->tables[table_id-1].dis);
    }
    pagenum_t freePagenum = header.H.freePnum;

    page_t tmp_free;  //첫번째 free page를 읽어오고
    buff_read_page(table_id,freePagenum,&tmp_free);

    header.H.freePnum = tmp_free.F.nfreePnum; //header page가 가리키는 free page 위치 변경해주고
    buff_write_page(table_id,0,&header); //header page의 변경사항 저장

    memset(&tmp_free,0,PAGE_SIZE);  //반환될 free page의 내용을 모두 지우고 넘겨줌
    buff_write_page(table_id,freePagenum,&tmp_free);


    return freePagenum;
}

void buff_free_page(int table_id, pagenum_t pagenum){
    buff_read_page(table_id, 0, &header);
    page_t tmp_page;
    buff_read_page(table_id,pagenum,&tmp_page); //free page 만들 page 읽어오고
    memset(&tmp_page,0,PAGE_SIZE); //전부 0으로 초기화
    
    tmp_page.F.nfreePnum = header.H.freePnum; //새로운 free page를 free page list의 맨 앞에 넣어주고
    header.H.freePnum = pagenum; //header page가 새로 만들어진 free page를 가리키게 해주고

    buff_write_page(table_id,pagenum,&tmp_page); //free page와 header page의 update한 값을 저장해줌
    buff_write_page(table_id,0,&header);
}

int buff_clean(void){
    int ret = 0;
    for(int i = 0; i < buff->used_cnt; i++){
        if(buff->blocks[i].is_pinned != 0){
            return -1;
            //pin 되어 있는 것이 있으면 할 동작
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
            
            if( buff->blocks[i].is_pinned != 0){
            return -1;
            //pin 되어 있는 것이 있으면 할 동작
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
    // printf("start compact\n");

    for(i=0; i<buff->used_cnt-1; i++){
        pthread_mutex_lock(&buff->blocks[i].page_latch);
    }
    // printf("all lock acq\n");

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

    for(i=0; i<buff->used_cnt-1; i++){
        pthread_mutex_unlock(&buff->blocks[i].page_latch);
    }
    
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
    ret = find_table_id(pathname); //못찾으면 -1
    if(ret == -1){
        ret = alloc_new_id(pathname); //할당 실패하면 -1
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