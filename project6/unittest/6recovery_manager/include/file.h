#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define VALUE_SIZE 120
#define PAGE_SIZE 4096
// #define LEAF_ORDER 32
// #define INTERNAL_ORDER 249
#define LEAF_ORDER 4
#define INTERNAL_ORDER 3

#define LOG_HEADER_SIZE 28
#define LOG_UPDR_SIZE 288
#define LOG_COMR_SIZE 296

#define LOG_BEG_TYPE 0
#define LOG_UPD_TYPE 1
#define LOG_COMM_TYPE 2
#define LOG_ROLL_TYPE 3
#define LOG_COM_TYPE 4

typedef uint64_t pagenum_t;

typedef struct headerPage{
    union
    {
        struct{
            pagenum_t freePnum;
            pagenum_t rootPnum;
            uint64_t numP;
        };
        char size[PAGE_SIZE];
    };
}headerPage;

typedef struct freePage{
    union
    {
        struct{
            pagenum_t nfreePnum;
        };
        char size[PAGE_SIZE];
    };
}freePage;

typedef struct Pheader{
    pagenum_t parent; 
    uint32_t isLeaf;
    uint32_t numKeys;
    char reserve[8];
    int64_t page_LSN;
    char reserve2[88];
    pagenum_t special;
}Pheader;

typedef struct leafRecord{
    int64_t key;
    char value[VALUE_SIZE];
}leafRecord;

typedef struct internalRecord{
    int64_t key;
    pagenum_t pageN;
}internalRecord;

typedef struct leafPage{
    union
    {
        struct{
            Pheader header;
            leafRecord records[LEAF_ORDER-1];
        };
        char size[PAGE_SIZE];
    };
}leafPage;

typedef struct internalPage{
    union
    {
        struct{
            Pheader header;
            internalRecord records[INTERNAL_ORDER-1];
        };
        char size[PAGE_SIZE];
    };
}internalPage;


typedef struct page_t {
    union
    {
        headerPage H;
        freePage F;
        internalPage I;
        leafPage L;
    };
}page_t;


typedef struct tmpleafPage{
    union
    {
        struct{
            Pheader header;
            leafRecord records[LEAF_ORDER];
        };
        char size[PAGE_SIZE];
    };
}tmpleafPage;

typedef struct tmpinternalPage{
    union
    {
        struct{
            Pheader header;
            internalRecord records[INTERNAL_ORDER];
        };
        char size[PAGE_SIZE];
    };
}tmpinternalPage;

typedef struct tmppage_t {
    union
    {
        tmpinternalPage I;
        tmpleafPage L;
    };
}tmppage_t;



#pragma pack(1)

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



typedef struct log_record_t {
    union
    {
        BCR_log_record BCR;
        UPDATE_log_record UPD;
        COMPENSATE_log_record COM;
    };
}log_record_t;
#pragma pack(8)

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(int file, pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(int file, pagenum_t pagenum, const page_t* src);

void file_expand_page(int table_id, int file);

int file_open_table(int table_id, int * file, char * pathname);

void makeHeaderPage(int table_id, int file);

int file_close_table(int file);


int log_file_open(int * file, char * pathname, log_record_t * last_record);
void log_file_write_record(int file, int start_point, int type, const log_record_t* src);
int log_file_read_record(int file, int start_point, int type, const log_record_t* dest);