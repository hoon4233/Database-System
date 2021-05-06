#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#define VALUE_SIZE 120
typedef uint64_t pagenum_t;

typedef struct BCR_log_record{
    int log_size;
    int64_t LSN;
    int64_t prev_LSN;
    int trx_id;
    int type;
}BCR_log_record;

typedef struct UPDATE_log_record{
    int log_size;
    int64_t LSN;
    int64_t prev_LSN;
    int trx_id;
    int type;

    int tid;
    pagenum_t page_num;
    int32_t offset;
    uint32_t data_length;
    char old_img[VALUE_SIZE];
    char new_img[VALUE_SIZE];
}UPDATE_log_record;

typedef struct COMPENSATE_log_record{
    int log_size;
    int64_t LSN;
    int64_t prev_LSN;
    int trx_id;
    int type;

    int tid;
    pagenum_t page_num;
    int32_t offset;
    uint32_t data_length;
    char old_img[VALUE_SIZE];
    char new_img[VALUE_SIZE];

    int64_t next_undo_LSN;
}COMPENSATE_log_record;

int main(void){
    
    printf("%d %d %d\n", sizeof(BCR_log_record), sizeof(UPDATE_log_record), sizeof(COMPENSATE_log_record));

    
    return 0;
}