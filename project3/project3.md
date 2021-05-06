# Project3 - Buffer Manager
## 목차
1. 디자인
   1. Layered Architecture
   2. Buffer
   3. Table
2. 코드 설명
   1. bpt
   2. file
   3. buffer
   4. table
3. 테스트 케이스
   1. Multi-table
   2. Insert
   3. Find
   4. Delete
   5. LRU replacement policy
   6. Close
   7. Shutdown
   

------

## 1. 디자인

### 1-1. Layered Architecture
```
**설명**
Buffer Manager를 구현하며 추가된 buffer/table layer에 대해 설명합니다.

사실상 buffer layer가 추가된 것이지만 여러 table을 관리하는 table manager가 필요하게 되었고 그 table manager를 초기화하고 작동시킬 여러 함수들이 필요하게 되었습니다.

여러 디자인을 고민하다 table manager는 상위 계층에 전역변수로 설정하고 table manager를 초기화하고 작동시킬 여러 함수들은 layered architecture에 위배되지 않도록 buffer에 함께 구현하였습니다.


***전체적인 코드는 2장에서 설명하겠습니다.***
``` 

![layered_arch](uploads/5fcc0eebd9fb581591ecb34d7e9ef71f/layered_arch.png)

<br/><br/>

### 1-2. Buffer

```c
//buffer
typedef struct block{
    page_t frame;
    int table_id;
    uint64_t page_num;
    int is_dirty;
    int is_pinned;
    struct block * LRU_next;
    struct block * LRU_pre;
}block;

typedef  struct buffer{
    block * blocks;
    block * LRU_head;
    block * LRU_tail;
    uint64_t capacity;
    uint64_t used_cnt;
}buffer;
```

```
**Buffer의 structure**
blocks 배열은 buffer의 한칸을 의미합니다.
LRU head와 tail은 LRU 정책으로 block을 replacement하기 위해 마련해놨습니다.
capacity는 buffer의 총 용량입니다.
used_cnt는 현재 buffer에 존재하는 block의 수입니다.

***buffer의 전체적인 동작 설명은 2-3에서 코드와 함께 하겠습니다.***
```
<br/><br/>

### 1-3. Table

```c
//buffer
typedef struct table{
    char name[20];
    int id;
    int dis;
}table;

typedef  struct tManager{
    table tables[10];
}tManager;
```

```
**Table의 structure**
Table Manager는 총 10개까지의 table을 가질 수 있습니다.
1개의 테이블 당
	name : 해당 table의 name을 저장합니다. table이 닫혔다가 다시 열렸을 경우 똑같은 id를 부여하기 위해 사용합니다.
	id : table의 id를 저장합니다. (1~10의 값을 갖습니다.)
	dis : file의 discriptor를 저장합니다.

***Table의 초기화 및 작동 함수의 설명은 2-4에서 코드와 함께 하겠습니다.***
```
<br/><br/>

------

## 2. 코드 설명

### 1. bpt
```
코드는 전체적으로 project2에서 구현한 b+tree의 코드를 이용하였고 바뀐점은 기존의 file_read_page, file_write_page 등 대신 buff_read_page, buff_write_page 등으로 disk가 아닌 buffer와 교류하도록 코드를 작성하였습니다.
이외에는 multi table 환경을 지원하기 위해서 함수의 parameter에 table id를 추가하였습니다.
또한 기존에 없었던 buffer를 위해 intit_db, shutdown_db 등의 함수를 추가하였습니다.
``` 
<br/><br/>

ex)
```c
int db_insert(int table_id, int64_t key, char * value);
int init_db(int num_buf);
pagenum_t delete_entry( int table_id, pagenum_t nN, int64_t key );
```
<br/><br/>

### 2. file
```
기존에 index layer가 아닌 buffer layer와 교류하기 위해 사용하는 parameter를 수정하였습니다.
타겟 table에 해당하는 file discriptor와 pagenum 등을 받아서 사용합니다.

또한 file의 크기가 변경될때 해당 변경사항을 disk의 header에 바로 적용하는 것이 아니라
buffer에 올라온(혹은 올려서) header에 변경사항을 먼저 반영하고
이후에 disk에 반영하기 위해서 buffer_write_page, buffer_free_page 함수가 사용됩니다.
layer들이 단방향 아닌 양방향으로 통신할 수 있다고 생각하여 해당 함수를 사용하여도 무관하다고 생각하였습니다.
``` 
<br/><br/>

ex)
```c
void file_read_page(int file, pagenum_t pagenum, page_t* dest)

void makeHeaderPage(int table_id, int file){
    memset(&header,0,PAGE_SIZE);
    file_write_page(table_id, file, &header);
    header.H.freePnum = 0;
    header.H.rootPnum = 0;
    header.H.numP = 1;
    printf("IN makeHeaderPage fd %d\n",file);
    buff_write_page(table_id, 0, &header);
    printf("IN makeHeaderPage fd %d\n",file);
}
```
<br/><br/>

### 3. buffer
```
이번 project에서 추가된 layer입니다.
기본적으로 상위 계층에서 지시받은 read와 write를 buffer에 쓰고
만약 해당 page가 없으면 disk에서 page를 가져옵니다.
이때 빈 slot이 없으면 LRU policy에 따라 victim을 선정합니다.
victim으로 선정된 page가 dirty page라면 disk에 변경사항을 저장하고 buffer에서 사라집니다.
(이때 buffer 안의 page들은 compact 됩니다.)
``` 
<br/><br/>

```c
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

void buffer_compact_page();
```
print_buff : buffer의 몇번째 slot에 어떤 table의 어떤 page가 있는지 출력합니다.

LRU_PUSH, LRU_POP : buffer manager의 LRU list에서 해당 block을 pop이나 push 합니다.

buff_evict_page : LRU policy에 따라 buffer에서 victim page를 선정하고 내려줍니다.
buff_find_page : buffer에 해당 table의 해당 page가 올라와 있는지 찾아줍니다.
buff_load_page : buffer에 해당 table의 해당 page를 올려줍니다.

init_buff : buffer를 초기화 해주는 함수입니다.
buff_read_page, write_page : 상위 계층에서 buffer에 읽고 쓰기 위한 함수입니다.
buff_alloc_page : 상위 계층에서 새로운 page가 필요할 경우 buffer에 page를 올려주는 함수입니다.
buff_free_page : buffer에 있는 해당 table의 해당 page를 free page로 만들어줍니다.

buff_clean : buffer를 완전히 비웁니다.
buff_flush_table : buffer에서 해당 table의 page들을 전부 내립니다.

buff_pin, unpin : 해당 page에 pin을 설정해주고 풀어주는 함수입니다.

buffer_compact_page : victim page를 없애고 buffer안의 page들을 compact 해주는 함수입니다.

ex)
```c
void buff_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }
    buff_pin(&buff->blocks[idx].is_pinned);

    memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);

    buff_unpin(&buff->blocks[idx].is_pinned);
}
```
해당 함수는 buffer에서 page를 읽어 상위 계층으로 보내주는 함수입니다.

먼저 buffer에서 해당 page가 존재하는지 찾고(buff_find_page)
해당 페이지가 있으면 그대로 진행을
없다면 해당 page를 disk에서 가져와 줍니다.(buff_load_page)

그리고 pin을 설정해주고
내용을 복사한 후
LRU list에서 update하는 과정을 거쳐줍니다.
이후 unpin을 한 후 동작을 종료합니다.

<br/><br/>

### 4. table
```
기존에 table이 1개였던 환경이 아닌 여러개의 table을 구동하기 위해 만들어진 table manager입니다.
``` 
<br/><br/>

```c
void init_tM(void);
int find_table_id(char * pathname);
int alloc_new_id(char * pathname);


int open_table_sec(char * pathname);
int close_table_sec(int table_id);
```
init_tM : table manager를 초기화 하는 함수입니다.
find_table_id : pathname으로 들어온 table이 기존에 열린적이 있던 table인지 확인하고 맞다면 open 해줍니다.
alloc_new_id : pathname으로 들어온 table이 처음 열리는 경우이고 table manager에서 빈 자리를 찾아 alloc 해줍니다.

open_table_sec : table manager를 이용해 open을 하면서 상위 계층에서 바로 file_open_table을 호출 할수 없기에 rapper function 같은 역할도 합니다.

open_table_sec : table이 닫힐 시 buffer에 있는 해당 table의 page를 모두 내려주고 (buff_flush_page 참조) 상위 계층에서 바로 file_close_table을 호출 할수 없기에 rapper function 같은 역할도 합니다.

<br/><br/>

------

## 3. 테스트 케이스
```
모든 test case는
buffer size = 3
leaf order = 4
internal order = 3  에서 진행되었습니다.
expand file 시 늘어나는 page = 5
(LRU replacement는 buffer size만 5로 변경)
(Close_table은 buffer size만 10으로 변경)
```

### 3-1. Multi-table
```
여러개의 table을 열 수 있나를 test 했습니다.
10개의 table을 열었고 10번째 table을 닫은 후 
11번째 table을 열고 10번째 table을 다시 열었을때
10개의 table 정확히 할당 -> 11번째 table open 실패 -> 10번째 table open시 원래 id 반환
이 가능한지 test 하였습니다.
``` 

![multi_table](uploads/895b0c34cdbd568b3313de36ed366d0b/multi_table.png)


### 3-2. Insert
```
두개의 table을 열고 10개씩 insertion 하였습니다.

buffer에 어떤 페이지가 들어있나 출력하였습니다.

insertion이 끝나고 난 후의 tree를 출력하였고
file을 닫은 후 다시 열어 tree를 출력해봄으로서
disk에 data가 제대로 저장되었는지 확인하였습니다.
``` 

![insert](uploads/a557b74ec40b8d46a4a07683a1d92d51/insert.png)

### 3-3. Find
```
두개의 table을 열고 10개씩 insertion 하였습니다.

insertion이 끝나고 난 후의 tree를 출력하였고

table a에서 key 3이 갖는 value 값을
table b에서 key 15가 갖는 value 값을
db_find하여 출력하였습니다.
``` 

![find](uploads/dae02ebef2d8e06b5f391bb587365a47/find.png)

### 3-4. Delete
#### 3-4-1 Delayed Merge
```
delete시 delayed merge가 제대로 일어나는지 확인하였습니다.

두개의 table을 열고 10개씩 insertion 하였습니다.

buffer에 어떤 페이지가 들어있나 출력하였습니다.

insertion이 끝나고 난 후의 tree를 출력하였고
deletion이 끝나고 난 후의 tree를 출력하였습니다.

file을 닫은 후 다시 열어 tree를 출력해봄으로서
disk에 data가 제대로 저장되었는지 확인하였습니다.
``` 

![delete_delay_merge](uploads/93eef80e0f5093494fe0bc4a8258edf7/delete_delay_merge.png)

#### 3-4-2 redistribution
```
delete시 merge를 못하는 상황에서 
redistribution이 제대로 일어나는지 확인하였습니다.

두개의 table을 열고 10개씩 insertion 하였습니다.

buffer에 어떤 페이지가 들어있나 출력하였습니다.

insertion이 끝나고 난 후의 tree를 출력하였고
deletion이 끝나고 난 후의 tree를 출력하였습니다.

file을 닫은 후 다시 열어 tree를 출력해봄으로서
disk에 data가 제대로 저장되었는지 확인하였습니다.
``` 

![delete_redistribution](uploads/f5f87bb862e6a7194d89bc86bf0ba62d/delete_redistribution.png)

### 3-5. LRU replacement policy
```
(LRU replacement는 buffer size만 5로 변경)

LRU 정책대로 replacement가 일어나는지 보기위해
처음 file이 expand되는 상황을 만들었습니다.

header page는 pagenum == 0이고
free page들은 1,2,3,4,5 순으로 만들어집니다.

buffer 크기가 5이니 pagenum이 5인 page가 생성될때
buffer에서 evict가 일어나야 합니다.

따라서 가장 최근에 사용되지 않은 pagenum 1인 page가
(header page는 free page를 만들면서 계속 접근되고 있으니까)
victim page가 되고 replacement가 일어납니다.
``` 

![LRU](uploads/29be69cf3f3b15aa19c732cd1fa68c84/LRU.png)

### 3-6. Close
```
(Close_table은 buffer size만 10으로 변경)

2개의 table을 열고 

첫번째 table에 1개의 insertion,
두번째 table에 2개의 insertion 후

close table을 이용해
buffer에 있는 table 1d의 page를 모두 내린 상황입니다.
``` 

![close](uploads/3aeb12741c6cb3d7c5f35a05085f824e/close.png)

### 3-7. Shutdown
```
1개의 table을 열고 1개의 insertion을 진행한 뒤

shutdown_db를 통해 buffer와 table을 없애고
다시 init_db, open_tabe로 buffer와 table을 생성한 뒤

1개의 record를 넣는 모습입니다.
``` 

![shutdown](uploads/195b7b3783f12e6d7d8e563e009c8648/shutdown.png)













