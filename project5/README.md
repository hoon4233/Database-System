# Project5 - Concurrency Control
## 목차
1. 디자인
   1. Transaction Manager
   2. Lock Manager
   3. wait-for graph
2. API
   1. db_find, db_update
   2. About Transaction
   3. About Lock
   4. About Deadlock detection
3. 시나리오
   1. 첫번째 시나리오
   2. 두번째 시나리오
   3. 세번째 시나리오
   4. 네번째 시나리오
   5. 다섯번째 시나리오
4. Unittest
   1. buff lock
   2. lock mode
   3. transaction manager
   4. wait-for graph
   

------

## 1. 디자인

### 1-1. Transaction Manager
```
**설명**
이번 프로젝트에 쓰일 Transaction Manager을 설명합니다.

그림은 transaction manager입니다.

transaction manager는 transaction들을 관리하고
transaction들은 각각 자신의 lock object를 가지고 있습니다.

***API는 2장에서 설명하겠습니다.***
``` 

![trxM](uploads/b00e96cc70c6cbbb8005ab114e3217c2/trxM.png)

<br/><br/>

### 1-2. Lock Manager
```
**설명**

lock object를 관리하는 lock manager입니다.

각각의 entry들은 hash table로 구현되었고
entry에 연결되어 있는 object들은 linked list로 구현되었습니다.

project4에서 사용한 lock manager를 활용하였습니다.

***API는 2장에서 설명하겠습니다.***
```

![lockM](uploads/2b4479ddc11afa6259f0170333cfde84/lockM.png)

<br/><br/>

### 1-3. wait-for graph

```
**설명**

deadlock detection에 사용할 wait-for graph 입니다.

graph manager는 vertex들을 관리하고
각각의 vertex들은 linked list로 구현되었습니다.

graph에 cycle이 존재하는지 확인할때는 bfs 알고리즘을 사용하였고
bfs 알고리즘을 위해 
Gqueue(알고리즘에 사용될 큐)와 visit Manager(알고리즘에 사용될 visit)
을 구현하였습니다.

Gqueue는 동적할당 정수형 배열을
visit Manager는 active한 transation을 vertex로 갖고
기다림을 의미하는 edge를 갖는 linked list로 만들었습니다.

***API는 2장에서 설명하겠습니다.***
```
![graph](uploads/59f730162353b6e06ef9e6a7f242337a/graph.png)

<br/><br/>

------

## 2. API

### 2-1. db_find, db_update
```
기존 프로젝트에서 사용했던 db_find를 활용하여
db_update를 만들었습니다.

이후 concurrency control을 위해 
db_find, db_update 내부를 조금 수정하고
buffer layer에서 쓰는 함수들도 일부 수정하였습니다.
``` 

```c
buff_read_page(table_id, leafN, &leaf);
    
    for( i = 0; i < leaf.L.header.numKeys; i++){
        if(leaf.L.records[i].key == key){
            lock_t * obj = lock_acquire(table_id, key, trx_id, 0, leaf.L.records[i].value);
            if(obj == 0){
                return 2;
            }
            buff_read_page(table_id, leafN, &leaf);
            memcpy(ret_val, leaf.L.records[i].value, VALUE_SIZE);
            return 0;
        }
    }
```
db_find의 내부입니다.<br/><br/>
기존에 buff_read_page로 page를 읽어오고 해당 키를 찾고 value 값을 return 했다면<br/><br/>
이제는 buff_read_page로 page를 읽어오고 해당 키가 있는지 확인하고
lock_acquire 함수를 호출해 record에 대한 lock을 획득 한 후
buff_read_page로 다시 최신의 page를 읽어온 다음 해당 하는 record를 return 합니다.
<br/><br/>

```c
buff_read_page(table_id, leafN, &leaf);
    
    for( i = 0; i < leaf.L.header.numKeys; i++){
        if(leaf.L.records[i].key == key){
            lock_t * obj = lock_acquire(table_id, key, trx_id, 1, leaf.L.records[i].value);
            if(obj == 0){
                // printf("IN db_update, lock acq fail\n");
                return 2;
            }
            buff_update_read_page(table_id, leafN, &leaf);
            memcpy(leaf.L.records[i].value, values ,VALUE_SIZE);
            buff_update_write_page(table_id, leafN, &leaf);
            return 0;
        }
    }
```
db_update의 내부입니다.<br/><br/>
db_find와 매우 유사하지만 다른점은
lock acquire 이후 buff_read_page와 buff_write_page가
atomic 하게 이루어져야 한다는 것입니다.<br/><br/>
따라서 1개의 읽고 쓰기를 한꺼번에 하는 함수를 둘로 나눠
buff_update_read_page, buff_update_write_page로 만들었습니다.
<br/><br/>

```c
void buff_update_read_page(int table_id, pagenum_t pagenum, page_t * dest){
    pthread_mutex_lock(&buff_latch);
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    if(idx == -1){
        idx = buff_load_page(table_id, pagenum);
    }
    pthread_mutex_lock(&buff->blocks[idx].page_latch);
    buff_pin(&buff->blocks[idx].is_pinned);

    memcpy(dest, &buff->blocks[idx].frame, PAGE_SIZE);
    LRU_POP(&buff->blocks[idx]);
    LRU_PUSH(&buff->blocks[idx]);
}

void buff_update_write_page(int table_id, pagenum_t pagenum, page_t * src){
    int idx = -1;
    idx = buff_find_page(table_id, pagenum);
    
    memcpy(&buff->blocks[idx].frame, src, PAGE_SIZE);
    buff->blocks[idx].is_dirty = 1;
    pthread_mutex_unlock(&buff_latch);

    buff_unpin(&buff->blocks[idx].is_pinned);
    pthread_mutex_unlock(&buff->blocks[idx].page_latch);
}
```
buff_update_read_page, buff_update_write_page의 내부입니다.<br/><br/>
먼저 buff_latch로 buffer 자체를 보호 한 뒤
buff_find_page, buff_load_page로 원하는 page를 찾거나 불러옵니다.<br/><br/>
이후 페이지를 읽거나 쓰고 page latch로 page를 보호 한뒤
buffer의 latch를 해제해 줍니다.
<br/><br/>

<br/><br/>
### 2-2. About Transaction
```
Transaction Manager를 구현하며 사용한 함수들에 대한 설명입니다.

init_trxM => Transaction Manager 초기화
trx_find => Transaction Manager에서 해당하는 Transaction 찾기

trx_begin => 새로운 Transaction 시작
trx_commit => Transaction이 가지고 있는 lock 모두 반환하고 commit

roll_back => Transaction이 수행한 모든 쓰기 작업 되돌리기
trx_abort => Transaction이 수행한 모든 쓰기 작업 되돌리고 Transaction 종료
``` 

```c
void init_trxM(void);
trx * trx_find(int trx_id);

int trx_begin(void);
int trx_commit(int trx_id);

int roll_back(int trx_id);
int trx_abort(int trx_id);
```
<br/><br/>

### 2-3. About Lock
```
Project4에서 구현한 Lock Table을
이번 project와 합치며 변경된 점에 대해서 설명하갰습니다.

**lock_acquire**
project4의 lock_acquire와 다르게
parameter로 trx_id, lock_mode, value를 추가적으로 받습니다.

trx_id는 transaction 관리를 위해
lock_mode는 s lock (0), x lock (1)을 사용하기 위해
value는 transaction이 abort 되는 경우에
x lock으로 변경된 값을 되돌리기 위해서 사용합니다.

lock_acquire 내부에서는 project4에서와 마찬가지로
lock_entry에 object를 추가합니다.
이후 project4와는 다르게 deadlock을 detection 한 후
deadlock이 아니라면 object를 반환해주거나 차례를 기다리며 잠에 듭니다.


**lock_release**
lock_release는 lock_acquire와 다르게
trx_table_latch와 lock_table_latch를 잡지 않는데
그 이유는 lock_release는 직접적으로 호출 되는 것이 아닌
trx_commit이나 trx_abort 등과 같이 이미 latch를 잡은 다른 함수들에서 호출되기 때문입니다.
``` 

```c
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, char * value);
int lock_release(lock_t* lock_obj);
```

<br/><br/>

### 2-3. About Deadlock detection
```
Project4와 달리 project5에서는 조건들이 변하며
deadlock이 일어날 수 있는 상황이 존재하게 되었습니다.

따라서 deadlock을 해결하기 위해 wait-for graph를 구현하였고
wait-for graph에서 cycle을 detection 해
deadlock을 detection 합니다.

wait-for graph는 vertex들의 linked list로 구현하였고
bfs 알고리즘을 이용해 cycle을 detection 합니다.

lock_acquire 내부에서 기다려야 하는 상황이라면
기다려야 하는 trx를 src_trx로 주고 기다림을 받는 trx를 dest_trx로 줍니다.
이후 add_edge로 단방향 edge를 그린 뒤 
src_trx를 check_cycle 함수의 인자로 주어 cycle을 detect합니다.
``` 

```c
void init_graphM(void);
void init_visitM(void);
void print_graphM(void);
Gnode * Gnode_find(int tar_vertex);
visit * visit_find(int tar_vertex);
Gnode * add_vertex(int tar_vertex);
void add_edge(int src, int dest);
void del_vertex(int tar_vertex);
void Genqueue( int edge );
int Gdequeue( void );
int check_cycle( int tar_vertex );
int detect_deadlock(int src_trx, int dest_trx);
```

<br/><br/>

------

## 3. 시나리오
```
모든 test case는
hash table size = 10
buffer size = 3
leaf order = 4
internal order = 3 에서 실행되었습니다.

sleep 함수를 이용해 trx 내부의 operation의 순서를 강제해
시나리오를 구성하였습니다.

main thread에서 insert를 위해 trx_id == 1 이 사용됩니다.
따라서 이후 발급되는 trx의 id는 2부터 시작입니다.

db_find 함수의 변경으로 인해 db_insert 시 trx_abrot가 일어나지만
전체 시나리오에는 영향을 주지 않습니다.
```

### 3-1. 첫번째 시나리오
```
10개의 trx들이 각각의 record에 update를 한 뒤
10개의 trx들이 각각의 record에 대해 find를 합니다.
``` 

![1](uploads/3a414e179ffd415bfd0cbdc99eebccb9/1.png)


### 3-2. 두번째 시나리오
```
1개의 record에 대해서
1개의 trx가 update를 한 후 sleep 합니다.
이때 다른 1개의 trx가 find를 하지만 lock을 획득하지 못하기 때문에
대기상태가 되고 이전에 sleep한 trx가 commit 한 이후에 동작하게 됩니다.
``` 

![2-1](uploads/85d84d08f8e4f20da9fa1e370ea717f3/2-1.png)
![2-2](uploads/07bc6af563610284b17a6453dc930f86/2-2.png)

### 3-3. 세번째 시나리오
```
trx1이 record1에 s lock을 걸고
trx2가 record2에 x lock을 겁니다.

이후 trx1이 record2에 s lock을 시도하고 대기하고
trx2가 record1에 x lock을 시도하므로서
deadlock이 일어나는 상황을 가정한 뒤

trx2가 abort 되며 roll_back이 제대로 수행되는지 확인하였습니다.
``` 

![3](uploads/c87fdb176a0e7b08f42e200d7773950d/3.png)

### 3-4. 네번째 시나리오
```
trx1이 record1에 x lock을 걸고
trx2가 record1에 s lock을 시도한 뒤 대기하는 상태에서
trx1이 record1에 s lock을 시도했을때

deadlock 상황이 아니라 trx1이 수행되고 commit 된 뒤
trx2가 임무를 수행하고 commit 하는 상황을 만들어보았습니다.
``` 

![4-1](uploads/03d90a73b197955c7e81765ddeae7776/4-1.png)
![4-2](uploads/3fa100a8365559030b09e72c903ecbf9/4-2.png)

### 3-5. 다섯번째 시나리오
```
trx1이 record1에 x lock을 걸고
trx1이 record1에 s lock을 걸었을때

deadlock 상황이 아니라 trx1이 제대로 수행되고
이후 commit 되는지 확인하였습니다.
``` 

![5](uploads/328c20c9ddc30e19b6700dc77ff2888b/5.png)

<br/><br/>

---

## 4. Unittest

### 4-1. buff lock
```
기존의 project3에서 buff_latch와 page_latch를 구현하였습니다.
또한 이번에 새로 필요한 db_update를 구현하였습니다.

record lock에 대한 concurrency를 보장하진 못하지만
buffer 자체나 page에 대한 concurrency를 확인하기 위해 구현하였습니다.

page를 읽고 쓰거나 page evict 등이 일어나는 등의 상황에서
buffer를 안전하게 사용할수 있는지 확인하였습니다.

new buff 폴더입니다.
``` 

### 4-2. lock mode
```
기존의 project4에서 lock_mode를 구현하였습니다.

project4에서는 1개의 record에 대해
오직 1개의 작업만이 수행될 수 있는 형태였는데

이를 s, x lock을 구현하므로써 
좀 더 효율적인 lock 사용이 가능해졌습니다.

lock mode 폴더입니다.
``` 

### 4-3. transaction manager
```
transaction manager를 구현하고 test 하였습니다.

transaction manager의 구조가 바뀌는 등
transaction manager가 제대로 작동하는지 보기위해 구현하였습니다.

trxM.c 파일입니다.
``` 

### 4-4. wait-for graph
```
deadlock detection을 위한 wait-for graph를 구현하고

graph가 원하는대로 작동하는지
cycle을 정확히 탐지해 낼 수 있는지 확인하였습니다.

waitgraph.c 파일입니다.
``` 