#include "file.h"
#include <pthread.h>
// #include <lock_table.h>

//buffer
typedef struct block{
    page_t frame;
    int table_id;
    uint64_t page_num;
    int is_dirty;
    int is_pinned;
    struct block * LRU_next;
    struct block * LRU_pre;

    pthread_mutex_t page_latch;
}block;

typedef  struct buffer{
    block * blocks;
    block * LRU_head;
    block * LRU_tail;
    uint64_t capacity;
    uint64_t used_cnt;
}buffer;


//table
typedef struct table{
    char name[20];
    int id;
    int dis;
}table;

typedef  struct tManager{
    table tables[10];
}tManager;


typedef struct log_buffer{
    int64_t global_LSN;
    int64_t global_prev_LSN;
    uint64_t capacity;
    uint64_t used_cnt;
    log_record_t * blocks;
}log_buffer;

//buffer
int open_table_sec(char * pathname);
int close_table_sec(int table_id);

void print_buff(void);

void LRU_PUSH(block * p);
void LRU_POP(block * p);

int buff_find_page(int table_id, pagenum_t pagenum);
void buff_evict_page();
int buff_load_page(int table_id, pagenum_t pagenum);

int init_buff(int num_buf);
void buff_read_page(int table_id, pagenum_t pagenum, page_t * dest);
void buff_write_page(int table_id, pagenum_t pagenum, page_t * src);
pagenum_t buff_alloc_page(int table_id);
void buff_free_page(int table_id, pagenum_t pagenum);

int buff_clean(void);
int buff_flush_table(int table_id);

void buff_pin(int * p);
void buff_unpin(int * p);
void buff_get_page_latch(int table_id, pagenum_t pagenum);
void buff_release_page_latch(int table_id, pagenum_t pagenum);

void buffer_compact_page();

void buff_read_page_new(int table_id, pagenum_t pagenum, page_t * dest);
void buff_update_read_page(int table_id, pagenum_t pagenum, page_t * dest);
void buff_update_write_page(int table_id, pagenum_t pagenum, page_t * src);


//table

void init_tM(void);
int find_table_id(char * pathname);
int alloc_new_id(char * pathname);


int open_table_sec(char * pathname);
int close_table_sec(int table_id);



//log_buff
int init_log_buff(int num_buf, char * log_path);
int log_buff_find();
void print_log_buff(void);
void log_buff_flush();
int log_buff_search_record(int file, int tar_LSN, const log_record_t* dest);
void log_buff_write(int64_t trx_prev_LSN, int trx_id, int type, int table_id,pagenum_t page_NUM, int index, char * old_img, char * new_img);
void log_buff_read_all_file(void);

void buff_read_page_no_lock(int table_id, pagenum_t pagenum, page_t * dest);
void buff_write_page_no_lock(int table_id, pagenum_t pagenum, page_t * src);