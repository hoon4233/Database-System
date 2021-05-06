#include "buffer.h"
#include <string.h>
#include <stdlib.h>

#define TABLE_NUM 10

extern buffer * buff;
extern tManager * tableM;
extern page_t header;
extern log_buffer * log_buff;
extern int log_file;
extern FILE * log_msg_file;

pthread_mutex_t buff_latch = PTHREAD_MUTEX_INITIALIZER;
extern pthread_mutex_t log_buff_latch;

//buffer
void print_buff(void){
    if(buff->used_cnt == 0){
        printf("Empty buffer\n");
    }
    else{
        
        for(int i =0; i<buff->used_cnt; i++){
            printf("idx : %d, table id : %d, page_num : %llu, page_LSN : %d\n",i,buff->blocks[i].table_id,buff->blocks[i].page_num, buff->blocks[i].frame.L.header.page_LSN);
            
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

    pthread_mutex_lock(&log_buff_latch);
    log_buff_flush();
    pthread_mutex_unlock(&log_buff_latch);

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
    printf("--------evcit ok----------\n");
    print_buff();
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
    // printf("IN rel_page_latch, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
    
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
    // printf("IN read_page_new, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
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
    // printf("IN update_read_page, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);
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
    // printf("IN update_write_page, table_id : %d, pagenum : %d, idx : %d\n",table_id, pagenum, idx);

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

    char num_name[3];
    memset(num_name,0,3);
    char first = pathname[4];
    char second = pathname[5];

    if(second == '0'){
        num_name[0] = first;
        num_name[1] = second;
        num_name[3] = '\0';
    }
    else{
        num_name[0] = first;
        num_name[1] = '\0';
    }

    int idx = atoi(num_name)-1;

    memcpy(tableM->tables[idx].name, pathname, 20);
    printf("IN alloc_new_id, tableM name %s, idx : %d, id :%d\n",tableM->tables[idx].name,idx, tableM->tables[idx].id);
    ret = file_open_table(tableM->tables[idx].id, &tableM->tables[idx].dis, pathname);

    return tableM->tables[idx].id;
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



//log_buff
int init_log_buff(int num_buf, char * log_path, char * logmsg_path){
    int ret = -1;
    log_buff = (log_buffer *)malloc(sizeof(log_buffer));
    log_buff->blocks = (log_record_t *) malloc(sizeof(log_record_t) * num_buf);
    log_buff->global_LSN = 0;
    // log_buff->global_prev_LSN = -1;
    log_buff->capacity = num_buf;
    log_buff->used_cnt = 0;
    
    if(log_buff == NULL || log_buff->blocks == NULL){
        return -1;
    }

    // open_log_file
    log_record_t last_record;
    memset(&last_record, 0, LOG_COMR_SIZE);
    int file_exist = -1;
    file_exist = log_file_open(&log_file, log_path, &last_record, logmsg_path);

    if(file_exist == -1){ //log_file_open error
        return -1;
    }
    if(file_exist == 1){ //log 파일이 이미 있는 경우
        log_buff->global_LSN = last_record.BCR.LSN + last_record.BCR.log_size;
        // log_buff->global_prev_LSN = last_record.BCR.LSN;
    }
    
    for(int i=0; i<num_buf; i++){
        log_buff->blocks[i].COM.log_size = -1;
        log_buff->blocks[i].COM.LSN = -1;
        log_buff->blocks[i].COM.prev_LSN = -1;
        log_buff->blocks[i].COM.trx_id = 0;
        log_buff->blocks[i].COM.type = -1;

        log_buff->blocks[i].COM.tid = 0;
        log_buff->blocks[i].COM.page_num = 0;
        log_buff->blocks[i].COM.offset = 0;
        log_buff->blocks[i].COM.data_length = VALUE_SIZE;
        memset(&log_buff->blocks[i].COM.old_img, 0, VALUE_SIZE);
        memset(&log_buff->blocks[i].COM.new_img, 0, VALUE_SIZE);

        log_buff->blocks[i].COM.next_undo_LSN = 0;
    }

    return 0;
}

int log_buff_find(void){
    int i;
    int idx = -1;
    for(i=0; i<log_buff->capacity; i++){
        if(log_buff->blocks[i].COM.log_size == -1){
            idx = i;
            return idx;
        }
    }
    return i;
}

void print_log_buff(void){
    if(log_buff->used_cnt == 0){
        printf("Empty buffer\n");
    }
    else{
        printf("Start print_log_buff, used_cnt : %d\n",log_buff->used_cnt);
        
        for(int i =0; i<log_buff->used_cnt; i++){
            printf("log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s, nex_undo : %d\n", log_buff->blocks[i].COM.log_size, log_buff->blocks[i].COM.LSN, log_buff->blocks[i].COM.prev_LSN, log_buff->blocks[i].COM.trx_id, log_buff->blocks[i].COM.type, log_buff->blocks[i].COM.tid, log_buff->blocks[i].COM.page_num, log_buff->blocks[i].COM.offset, log_buff->blocks[i].COM.data_length, log_buff->blocks[i].COM.old_img, log_buff->blocks[i].COM.new_img, log_buff->blocks[i].COM.next_undo_LSN );
            
        }
    }
    printf("END print_log_buff\n");
}

void log_buff_flush(){
    // pthread_mutex_lock(&slog_buff_latch);

    // printf("IN log_buff_flush, used_cnt : %d\n",log_buff->used_cnt);
    for(int i = 0; i < log_buff->used_cnt; i++){
        log_record_t tmp_record;
        memset(&tmp_record, 0, LOG_COMR_SIZE);
        memcpy(&tmp_record, &log_buff->blocks[i].COM, LOG_COMR_SIZE);
        log_file_write_record(log_file, log_buff->blocks[i].COM.LSN, log_buff->blocks[i].COM.type, &tmp_record);

        log_buff->blocks[i].COM.log_size = -1;
        log_buff->blocks[i].COM.LSN = -1;
        log_buff->blocks[i].COM.prev_LSN = 0;
        log_buff->blocks[i].COM.trx_id = 0;
        log_buff->blocks[i].COM.type = -1;

        log_buff->blocks[i].COM.tid = 0;
        log_buff->blocks[i].COM.page_num = 0;
        log_buff->blocks[i].COM.offset = 0;
        log_buff->blocks[i].COM.data_length = VALUE_SIZE;
        memset(&log_buff->blocks[i].COM.old_img, 0, VALUE_SIZE);
        memset(&log_buff->blocks[i].COM.new_img, 0, VALUE_SIZE);

        log_buff->blocks[i].COM.next_undo_LSN = 0;
    }

    log_buff->used_cnt = 0;
    
    // pthread_mutex_unlock(&log_buff_latch);
}


int log_buff_search_record(int file, int tar_LSN, const log_record_t* dest){
    int ret = -1;
    for(int i = 0; i < log_buff->used_cnt; i++){
        if(log_buff->blocks[i].BCR.LSN == tar_LSN ){
            memcpy(dest, &log_buff->blocks[i].COM, LOG_COMR_SIZE);
            return -3;
        }
    }
    ret = log_file_read_record(file, tar_LSN, 0, dest);
    return ret;
}


void log_buff_write(int64_t trx_prev_LSN, int trx_id, int type, int table_id, pagenum_t page_NUM, int index, char * old_img, char * new_img){
    int idx = -1;
    idx = log_buff_find();

    if(idx == log_buff->capacity){
        log_buff_flush();
        idx = 0;
    }

    log_buff->blocks[idx].BCR.log_size = LOG_HEADER_SIZE;
    log_buff->blocks[idx].BCR.LSN = log_buff->global_LSN;
    log_buff->blocks[idx].BCR.prev_LSN = trx_prev_LSN;
    log_buff->blocks[idx].BCR.trx_id = trx_id;
    log_buff->blocks[idx].BCR.type = type;

    if(type == LOG_UPD_TYPE){
        log_buff->blocks[idx].UPD.log_size = LOG_UPDR_SIZE;

        log_buff->blocks[idx].UPD.tid = table_id;
        log_buff->blocks[idx].UPD.page_num = page_NUM;
        log_buff->blocks[idx].UPD.offset = 128+(128*index)+8;
        log_buff->blocks[idx].UPD.data_length = VALUE_SIZE;
        memcpy(&log_buff->blocks[idx].UPD.old_img, old_img, VALUE_SIZE);
        memcpy(&log_buff->blocks[idx].UPD.new_img, new_img, VALUE_SIZE);
    }
    if(type == LOG_COM_TYPE){
        log_buff->blocks[idx].COM.log_size = LOG_COMR_SIZE;

        log_buff->blocks[idx].COM.tid = table_id;
        log_buff->blocks[idx].COM.page_num = page_NUM;
        log_buff->blocks[idx].COM.offset = 128+(128*index)+8;
        log_buff->blocks[idx].COM.data_length = VALUE_SIZE;
        memcpy(&log_buff->blocks[idx].UPD.old_img, old_img, VALUE_SIZE);
        memcpy(&log_buff->blocks[idx].UPD.new_img, new_img, VALUE_SIZE);


        //set next_undo point
        log_record_t prev_log;  //지금 발급하는 log의 바로 이전 log
        memset(&prev_log, 0, LOG_COMR_SIZE);
        // int ret = log_file_read_record(log_file, trx_prev_LSN, 0, &prev_log );
        int ret =log_buff_search_record(log_file, trx_prev_LSN, &prev_log );

        // printf("IN log_buff_write, for undo point, trx_prev_LSN : %d, prev_record's type : %d\n",trx_prev_LSN, prev_log.BCR.type);

        if(prev_log.BCR.type == LOG_UPD_TYPE){
            log_record_t pre_prev_log;  //지금 발급하는 log의 바로 이전 log의 이전 log
            memset(&pre_prev_log, 0, LOG_COMR_SIZE);
            // ret = log_file_read_record(log_file, prev_log.BCR.prev_LSN, 0, &pre_prev_log );
            ret = log_buff_search_record(log_file, prev_log.BCR.prev_LSN, &pre_prev_log );

            if(pre_prev_log.BCR.type == LOG_BEG_TYPE){
                log_buff->blocks[idx].COM.next_undo_LSN = -1;
            }
            if(pre_prev_log.BCR.type == LOG_UPD_TYPE){
                log_buff->blocks[idx].COM.next_undo_LSN = pre_prev_log.BCR.LSN;
            }
        }

        if(prev_log.BCR.type == LOG_COM_TYPE){
            log_record_t tmp_next_undo; //지금 발급하는 log의 바로 이전 log가 가리키는 next_undo point
            memset(&tmp_next_undo, 0, LOG_COMR_SIZE);
            // ret = log_file_read_record(log_file, prev_log.COM.next_undo_LSN, 0, &tmp_next_undo);
            ret = log_buff_search_record(log_file, prev_log.COM.next_undo_LSN, &tmp_next_undo);

            log_record_t tar_log; //지금 발급하는 log의 바로 이전 log가 가리키는 next_undo point의 바로 이전 log
            memset(&tar_log, 0, LOG_COMR_SIZE);
            // ret = log_file_read_record(log_file, tmp_next_undo.COM.prev_LSN, 0, &tar_log);
            ret = log_buff_search_record(log_file, tmp_next_undo.COM.prev_LSN, &tar_log);

            if(tar_log.BCR.type == LOG_BEG_TYPE){
                log_buff->blocks[idx].COM.next_undo_LSN = -1;
            }
            if(tar_log.BCR.type == LOG_UPD_TYPE){
                log_buff->blocks[idx].COM.next_undo_LSN = tar_log.BCR.LSN;
            }
        }

        // log_buff->blocks[idx].COM.next_undo_LSN = 0; //고민해보기
    }


    //set global_LSN
    log_buff->global_LSN = log_buff->blocks[idx].BCR.LSN+log_buff->blocks[idx].BCR.log_size;

    //set glbal_prev_lsn
    // if(log_buff->blocks[idx].BCR.LSN == 0){
    //     log_buff->global_prev_LSN = -1;
    // }
    // else{
    //     log_buff->global_prev_LSN = log_buff->global_LSN-log_buff->blocks[idx].BCR.log_size;
    // }

    log_buff->used_cnt += 1;
    
}

void log_buff_read_all_file(void){

    int idx = 0;
    int ret = 0;
    off_t offset = 0;
    
    printf("START log_buff_read_all_file\n");
    log_buff->blocks[log_buff->capacity].BCR.type = 0;
    while(ret != -1){
        idx+=1;
        ret = log_file_read_record(log_file, offset, log_buff->blocks[log_buff->capacity].BCR.type, &log_buff->blocks[log_buff->capacity]);
        if(ret == -2){
            printf("log_file is empty end\n");
            return;
        }
        if(log_buff->blocks[log_buff->capacity].BCR.type == LOG_BEG_TYPE || log_buff->blocks[log_buff->capacity].BCR.type == LOG_COMM_TYPE || log_buff->blocks[log_buff->capacity].BCR.type == LOG_ROLL_TYPE){
            offset += LOG_HEADER_SIZE;
            printf("%dst record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d  \n", idx,log_buff->blocks[log_buff->capacity].BCR.log_size, log_buff->blocks[log_buff->capacity].BCR.LSN, log_buff->blocks[log_buff->capacity].BCR.prev_LSN, log_buff->blocks[log_buff->capacity].BCR.trx_id, log_buff->blocks[log_buff->capacity].BCR.type );
        }
        if(log_buff->blocks[log_buff->capacity].BCR.type == LOG_UPD_TYPE){
            offset += LOG_UPDR_SIZE;
            printf("%dst record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d  \n", idx,log_buff->blocks[log_buff->capacity].BCR.log_size, log_buff->blocks[log_buff->capacity].BCR.LSN, log_buff->blocks[log_buff->capacity].BCR.prev_LSN, log_buff->blocks[log_buff->capacity].BCR.trx_id, log_buff->blocks[log_buff->capacity].BCR.type );
            printf("%dst record tid : %d, pag_num: %d, offset : %d, data_length : %d, old_img : %s, new_img : %s  \n", idx,log_buff->blocks[log_buff->capacity].UPD.tid, log_buff->blocks[log_buff->capacity].UPD.page_num, log_buff->blocks[log_buff->capacity].UPD.offset, log_buff->blocks[log_buff->capacity].UPD.data_length, log_buff->blocks[log_buff->capacity].UPD.old_img, log_buff->blocks[log_buff->capacity].UPD.new_img );
        }
        if(log_buff->blocks[log_buff->capacity].BCR.type == LOG_COM_TYPE){
            offset += LOG_COMR_SIZE;
            printf("%dst record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d  \n", idx,log_buff->blocks[log_buff->capacity].COM.log_size, log_buff->blocks[log_buff->capacity].COM.LSN, log_buff->blocks[log_buff->capacity].COM.prev_LSN, log_buff->blocks[log_buff->capacity].COM.trx_id, log_buff->blocks[log_buff->capacity].COM.type );
            printf("%dst record tid : %d, page_num : %d, offset : %d, data_length : %d, old_img : %s, new_img : %s, nex_undo LSN : %d  \n", idx,log_buff->blocks[log_buff->capacity].COM.tid, log_buff->blocks[log_buff->capacity].COM.page_num, log_buff->blocks[log_buff->capacity].COM.offset, log_buff->blocks[log_buff->capacity].COM.data_length, log_buff->blocks[log_buff->capacity].COM.old_img, log_buff->blocks[log_buff->capacity].COM.new_img, log_buff->blocks[log_buff->capacity].COM.next_undo_LSN );
        }
        printf("\n");
        

    }
    printf("END log_buff_read_all_file\n");
    
}

void buff_read_page_no_lock(int table_id, pagenum_t pagenum, page_t * dest){
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

void buff_write_page_no_lock(int table_id, pagenum_t pagenum, page_t * src){
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