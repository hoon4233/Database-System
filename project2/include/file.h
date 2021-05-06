#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define VALUE_SIZE 120
#define PAGE_SIZE 4096
#define LEAF_ORDER 32
#define INTERNAL_ORDER 249
// #define LEAF_ORDER 4
// #define INTERNAL_ORDER 5

#ifndef O_DIRECT
#define O_DIRECT 00040000
#endif

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
    char reserve[104];
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



// Allocate an on-disk page from the free page list
pagenum_t file_alloc_page();

// Free an on-disk page to the free page list
void file_free_page(pagenum_t pagenum);

// Read an on-disk page into the in-memory page structure(dest)
void file_read_page(pagenum_t pagenum, page_t* dest);

// Write an in-memory page(src) to the on-disk page
void file_write_page(pagenum_t pagenum, const page_t* src);

void file_expand_page(void);

int file_open_talbe(char * pathname);

void makeHeaderPage(void);