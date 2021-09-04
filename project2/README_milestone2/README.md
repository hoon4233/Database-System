# Project2 - disk-based b+tree (Milestone 2)
## 목차
1. Page 구조체
   1. Header Page
   2. Free Page
   3. Internal Page
   4. Leaf Page
   5. page_t
   6. tmppage_t
2. File Manager API
   1. file_alloc_page
   2. file_free_page
   3. file_read_page
   4. file_write_page
   5. file_expand_page
   6. file_open_table
   7. makeHeaderPage
3. Open, Insert, Find, Delete
   1. Open
   2. Insert
   3. Find
   4. Delete
      1. Delete 전체 디자인
      2. Delayed Merge
   
   **모든 실행 예시는 INTERNAL ORDER == 5, LEAF ORDER == 4에서 실행되었습니다.**

------

## 1. Page 구조체
```
코드 전반에 사용될 page들의 구조체에 대해 설명해보겠습니다.
```

### 1-1. Header Page

**Table의 정보를 담고 있는 Header Page 구조체**

```c
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
```

freePnum = 다음에 사용할 free page 번호를 담고 있습니다.<br/><br/>
rootPnum = 현재 root page 번호를 담고 있습니다.<br/><br/>
numP = 현재 file에 존재하는 page 개수를 담고 있습니다.<br/><br/>
size = Page Size를 맞춰주기 위해 사용하였습니다.<br/><br/>

### 1-2. Free Page

**Free Page 구조체**

```c
typedef struct freePage{
    union
    {
        struct{
            pagenum_t nfreePnum;
        };
        char size[PAGE_SIZE];
    };
}freePage;
```

nfreePnum = 다음 free page 번호를 담고 있습니다.<br/><br/>
size = Page Size를 맞춰주기 위해 사용하였습니다.<br/><br/>

### 1-3. Internal Page

**Internal Page 구조체**

```c
typedef struct Pheader{
    pagenum_t parent; 
    uint32_t isLeaf;
    uint32_t numKeys;
    char reserve[104];
    pagenum_t special;
}Pheader;

typedef struct internalRecord{
    int64_t key;
    pagenum_t pageN;
}internalRecord;

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
```
Pheader의 parnet = page의 parent의 page 번호를 담고 있습니다.<br/><br/>
Pheader의 isLeaf = page가 Interanl page인지 Leaf page인지를 저장합니다.<br/><br/>
Pheader의 reserve = 이번 프로젝트에서 사용하지 않는 공간입니다.<br/><br/>
Pheader의 special = page의 left most child page의 page 번호를 담고 있습니다.<br/><br/>

internalRecord의 key = Internal page의 key를 담습니다.<br/><br/>
internalRecord의 pageN = Internal page의 자식 page의 page 번호를 담습니다.<br/><br/>

internalPage의 header = Internal page header로써 Pheader 구조체를 이용합니다.<br/><br/>
internalPage의 records = Internal record들로써 internalRecord 구조체를 이용하고 order수보다 1개 적게 선언해줍니다.<br/><br/>

### 1-4. Leaf Page

**Leaf Page 구조체**

```c
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
```
Pheader의 parnet = page의 parent의 page 번호를 담고 있습니다.<br/><br/>
Pheader의 isLeaf = page가 Interanl page인지 Leaf page인지를 저장합니다.<br/><br/>
Pheader의 reserve = 이번 프로젝트에서 사용하지 않는 공간입니다.<br/><br/>
Pheader의 special = page의 right sibling page의 page 번호를 담고 있습니다.<br/><br/>

leafRecord의 key = Leaf page의 key를 담습니다.<br/><br/>
leafRecord의 value = Leaf page의 value를 담습니다.<br/><br/>

leafPage의 header = leaf page header로써 Pheader 구조체를 이용합니다.<br/><br/>
leafPage의 records = leaf page record들로써 leafRecord 구조체를 이용하고 order수보다 1개 적게 선언해줍니다.<br/><br/>

### 1-5. page_t

**page_t 구조체**

```c
typedef struct page_t {
    union
    {
        headerPage H;
        freePage F;
        internalPage I;
        leafPage L;
    };
}page_t;
```

page_t 구조체 안에 지금까지 설명해왔던 구조체들의 변수를 하나씩 선언함으로써 해당 페이지를 상황에 맞게 사용합니다.<br/><br/>
ex) page_t a; a.H or a.F ...<br/><br/>

<br/><br/>

### 1-6. tmppage_t

**tmppage_t 구조체**

```c
typedef struct page_t {
    union
    {
        headerPage H;
        freePage F;
        internalPage I;
        leafPage L;
    };
}page_t;
```

Insertion시 사용될 tmppage_t입니다. 기존의 page와 다른점이 있다면 Internal page와 Leaf page에서 record를 기존 page보다 1개 더 가지고 있습니다..<br/><br/>
ex) 즉 Leaf에서는 32개, Internal에서는 249개 <br/><br/>

<br/><br/>

------

### 2. File Manager API
```
코드 전반에 사용될 File Manager API에 대해 설명해보겠습니다.
File manager API를 사용함으로써 우리는 다른 함수에서 file을 열거나 늘리거나,
file에 page를 할당하거나 page를 free page로 만들거나,
file로부터 page를 읽고 쓰는 일들을
다른 함수내에서 신경쓰지 않고 1개의 함수 호출로 해결할 수 있습니다.
``` 

### 2-1. file_alloc_page

**Free Page를 alloc 해주는 함수**

```c
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
```

free page가 없는 경우 file_expand_page로 free page를 늘려주고 header page의 다음 free page를 변경시켜준 후 1개의 free page number를 반환해줍니다.<br/><br/>

### 2-2. file_free_page

**Page를 free page로 만들어주는 함수**

```c
void file_free_page(pagenum_t pagenum){
    page_t tmp_page;
    file_read_page(pagenum,&tmp_page); //free page 만들 page 읽어오고
    memset(&tmp_page,0,PAGE_SIZE); //전부 0으로 초기화
    
    tmp_page.F.nfreePnum = header.H.freePnum; //새로운 free page를 free page list의 맨 앞에 넣어주고
    header.H.freePnum = pagenum; //header page가 새로 만들어진 free page를 가리키게 해주고

    file_write_page(pagenum,&tmp_page); //free page와 header page의 update한 값을 저장해줌
    file_write_page(0,&header);
}
```

인자로 들어온 page number에 해당하는 page를 free page로 만들어줍니다.<br/><br/>

### 2-3. file_read_page

**Page를 read 해주는 함수**

```c
void file_read_page(pagenum_t pagenum, page_t* dest){
    off_t offset = PAGE_SIZE * pagenum;
    lseek(file, offset, SEEK_SET);
    read(file, dest, PAGE_SIZE);
}
```

인자로 들어온 page number에 해당하는 page에 해당하는 offset으로 이동하여 Page size만큼 읽어 dest에 저장해줍니다.<br/><br/>

### 2-4. file_write_page

**Page를 write 해주는 함수**

```c
void file_write_page(pagenum_t pagenum, const page_t* src){
    off_t offset = PAGE_SIZE * pagenum;
    lseek(file, offset, SEEK_SET);
    write(file, src, PAGE_SIZE);
}
```

인자로 들어온 page number에 해당하는 page에 해당하는 offset으로 이동하여 Page size만큼 src의 내용을 file에 저장해줍니다.<br/><br/>

### 2-5. file_write_page

**File의 크기를 늘려주는 함수**

```c
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
```

File에 더이상 free page가 없을 경우 file의 크기를 늘려줍니다.<br/><br/>

### 2-6. file_open_table

**File을 열어주는 함수**

```c
int file_open_talbe(char * pathname){
    file = open(pathname,O_RDWR|O_SYNC|O_DIRECT);
    if(file<0){  //처음 database를 생성하는 경우
        file = open(pathname, O_CREAT|O_RDWR|O_SYNC|O_DIRECT, S_IRWXU);
        if(file<0){
            printf("Creating database is failed\n");
            return -1;
        }
        makeHeaderPage();
    }
    
    file_read_page(0,&header); //이미 있는 database를 가져온 경우

    return 0;
}
```

pathname으로 file의 이름을 받아 해당 file을 열어주고 만약 해당 file이 존재하지 않다면 file을 만들어줍니다.<br/><br/>

### 2-7. makeHeaderPage

**File을 새로 만들시 header page를 만들어주는 함수**

```c
void makeHeaderPage(void){
    memset(&header,0,PAGE_SIZE);
    header.H.freePnum = 0;
    header.H.rootPnum = 0;
    header.H.numP = 1;
    file_write_page(0, &header);
}
```

file을 새로 생성될때 header page를 만들어줍니다.<br/><br/>

------

## 3. Open, Insert, Find, Delete
```
구현해야할 open_table, db_insert, db_find, db_delete에 대해 설명해보겠습니다.
```

### 3-1. open_table

**pathname으로 받을 file을 열어주는 함수**

```c
int open_table(char* pathname){
    int tmp_tableNum = tableNum;
    tableNum += 1;

    if( file_open_talbe(pathname) == -1 )
        return -1;

    return tmp_tableNum;
}
```

file을 열어주고 해당 table의 unique table id를 return 해줍니다. open이 실패할 경우 -1를 return 해줍니다.<br/><br/>

### 3-2. Insert

**Table에 key와 value를 insert 하는 함수입니다.**
```
전체 logic은 제공받은 bpt.c에 insert 함수를 따랐고
내부에서 사용하는 구조체만 1-1부터 1-5에서 설명한 구조체를 이용하였습니다.
``` 


```c
int db_insert(int64_t key, char* value){
    if (file < 0){
        printf("open_table fail\n");
        return 1;
    }
    char * ret_val = (char*)malloc(sizeof(char)*120);
    pagenum_t rootN;
    if(db_find(key,ret_val) == 0 ){
        return 1;
    }

    if(header.H.rootPnum == 0){
        start_new_tree(key, value);
        return 0;
    }

    pagenum_t leaf_pageN = find_leaf(key);
    page_t leaf_page;
    file_read_page(leaf_pageN,&leaf_page);

    if(leaf_page.L.header.numKeys < LEAF_ORDER-1){
        rootN = insert_into_leaf(leaf_pageN, key, value);
        return 0;
    }

    rootN = insert_into_leaf_after_splitting(leaf_pageN, key, value);
    return 0;

}
```

insert를 해주는 db_insert 함수입니다.<br/><br/>

```
pagenum_t make_internal( void );
pagenum_t make_leaf( void );
void start_new_tree(int64_t key, char * value);
pagenum_t find_leaf(int64_t key);
pagenum_t insert_into_leaf( pagenum_t leaf_pageN, int64_t key, char * value );
pagenum_t insert_into_leaf_after_splitting( pagenum_t leaf_pageN, int64_t key,
                                        char * value);

int cut( int length );
pagenum_t insert_into_parent( pagenum_t leftN, int64_t key, pagenum_t rightN);
pagenum_t insert_into_new_root(pagenum_t leftN, int64_t key, pagenum_t rightN);

int get_left_index(pagenum_t parentN, pagenum_t leftN);

pagenum_t insert_into_node( pagenum_t nN, 
        int left_index, int64_t key, pagenum_t rightN);
pagenum_t insert_into_node_after_splitting( pagenum_t old_pageN, int left_index, 
        int64_t key, pagenum_t rightN);
```

db_insert를 구현하기 위해 사용된 함수들입니다.<br/><br/>

**실행 예시**

<img width="107" alt="2-1" src="https://user-images.githubusercontent.com/26400022/132084137-9c0c545c-cb4f-45a7-873a-37088405566b.png"><br/><br/>

<img width="179" alt="2-2" src="https://user-images.githubusercontent.com/26400022/132084167-a002d8c1-7f8a-4204-bee8-ad766307570c.png"><br/><br/>
leaf page가 split 되는 경우 입니다.(internal page에 insert 되는 경우)<br/><br/>

<img width="225" alt="2-3" src="https://user-images.githubusercontent.com/26400022/132084177-3c3bfd84-60a9-45e5-9397-b693c920c27e.png"><br/><br/>
internal page에 insert 되는 경우 입니다.(leaf page가 split 되는 경우)<br/><br/>

<img width="482" alt="2-4" src="https://user-images.githubusercontent.com/26400022/132084203-fb84ab6a-df47-43aa-b590-bb1303666efd.png"><br/><br/>
internal page가 split 되는 경우 입니다.<br/><br/>

### 3-3. Find

**Table에서 key의 value를 find 하는 함수입니다.**
```
전체 logic은 제공받은 bpt.c에 find 함수를 따랐고
내부에서 사용하는 구조체만 1-1부터 1-5에서 설명한 구조체를 이용하였습니다.
``` 


```c
int db_find( int64_t key, char * ret_val ) {
    if (file < 0){
        printf("open_table fail\n");
        return 1;
    }
    int i = 0;
    pagenum_t leafN = find_leaf( key );
    if(leafN == 0){
        return 1;
    }
    page_t leaf;
    file_read_page(leafN, &leaf);
    
    for( i = 0; i < leaf.L.header.numKeys; i++){
        if(leaf.L.records[i].key == key){
            memcpy(ret_val, leaf.L.records[i].value, VALUE_SIZE);
            return 0;
        }
    }
    
    return 1;
}
```

find를 해주는 db_find 함수입니다.<br/><br/>

```
pagenum_t find_leaf(int64_t key);
```

db_find를 구현하기 위해 사용된 함수들입니다.<br/><br/>

**실행 예시**

<img width="519" alt="2-5" src="https://user-images.githubusercontent.com/26400022/132084204-3e36dbfd-ea5b-4ba5-8c6a-50a104076602.png"><br/><br/>
find의 실행 예시입니다.<br/><br/>

### 3-4. Delete

#### 3-4-1. Delete 전체 디자인

**Table에 key와 value를 delete 하는 함수입니다.**
```
전체 logic은 제공받은 bpt.c에 delete 함수를 따랐고
내부에서 사용하는 구조체만 1-1부터 1-5에서 설명한 구조체를 이용하였습니다.
``` 


```c
int db_delete( int64_t key ) {
    if (file < 0){
        printf("open_table fail\n");
        return 1;
    }
    pagenum_t rootN;
    char * ret_val = (char*)malloc(sizeof(char)*120);
    pagenum_t key_leaf = find_leaf(key);
    
    if ( db_find(key,ret_val) == 0 && key_leaf != 0) {
        rootN = delete_entry(key_leaf, key);
        free(ret_val);
        return 0;
    }
    free(ret_val);
    return 1;
}
```

delete를 해주는 db_delete 함수입니다.<br/><br/>

```
pagenum_t delete_entry( pagenum_t nN, int64_t key );
pagenum_t adjust_root(pagenum_t rootN);
int get_neighbor_index( pagenum_t nN );
pagenum_t coalesce_nodes( pagenum_t nN, pagenum_t neighborN, int neighbor_index, int64_t k_prime);
pagenum_t redistribute_nodes( pagenum_t nN, pagenum_t neighborN, int neighbor_index, 
        int k_prime_index, int64_t k_prime);
```

db_delete를 구현하기 위해 사용된 함수들입니다.<br/><br/>

#### 3-4-2. Delayed Merge

**Page의 내용물이 모두 삭제 되었을때 비로소 Merge하는 정책입니다.**
```
이전의 bpt와는 다르게 disk i/o를 줄이는 등 여러가지 이점을 얻기 위해
본 project 에서는 Delayed Merge가 구현되었습니다.
따라서 Page의 내용물이 모두 사라졌을때 그제서야 merge를 실행하게 됩니다.
``` 

**실행 예시**

<img width="521" alt="2-6" src="https://user-images.githubusercontent.com/26400022/132084205-cc65dec2-1d6d-4445-b538-b40e9b20b6bd.png"><br/><br/>
leaf에서 삭제되는 경우입니다.<br/><br/>

<img width="444" alt="2-7" src="https://user-images.githubusercontent.com/26400022/132084206-77a9b2ff-56e6-460f-9244-673999f2a11c.png"><br/><br/>
leaf가 모두 삭제되어 merge 되는 경우입니다.<br/><br/>

<img width="289" alt="2-8" src="https://user-images.githubusercontent.com/26400022/132084207-6487bfb4-efed-4b56-864e-9a30cafbcfa5.png"><br/><br/>
internal이 모두 삭제되고 neighbor와 merge 하는 경우 입니다.<br/><br/>

<img width="694" alt="2-9" src="https://user-images.githubusercontent.com/26400022/132084208-82fedbe8-71db-4bbe-aa1c-555370c73b59.png"><br/><br/>
internal이 모두 삭제되었지만 neighbor가 가득차 merge를 하지 못하고 redistribution 하는 경우입니다.<br/><br/>