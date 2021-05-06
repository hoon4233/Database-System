# Project6 - Transaction Logging & Three-pass Recovery
## 목차
1. 디자인
   1. General Lock
   2. Log 읽고 쓰기
   3. Recovery (Three-pass Recovery)
2. API
   1. General Lock
   2. Log 읽고 쓰기
   3. Recovery
3. Test Result
   1. 상황1
   2. 상황2를 재실행 하였을 때
   3. flag 1
   4. flag 2
4. 문제점
   

------

## 1. 디자인

### 1-1. General Lock
```
**설명**
project5와는 다른 순서로 lock을 잡고 있습니다.

간단하게 정리해
기존에는 buffer lock 잡고 -> page lock 잡고 -> page lock를 풀고 record lcok를 잡고
다시 buffer lock 잡고 - page lock을 잡는 순이였고

record lock을 잡을 수 없을땐 page lock을 풀고 기다리는 형식이였지만

이번 project부턴 lock을 잡을 수 없는 상황에서
page lock을 바로 푸는 것이 아니라 trx talbe lock을 잡고
해당 trx의 lock을 잡고 
page와 lock manager의 lock을 풀고
해당 trx의 lock을 반납하는 동시에 자는 구조를 취합니다.

***API는 2장에서 설명하겠습니다.***
``` 

![스크린샷_2020-12-17_오후_10.13.12](uploads/d0617e35cbd599f9c487f93e5ab40f3b/스크린샷_2020-12-17_오후_10.13.12.png)
![스크린샷_2020-12-17_오후_10.13.01](uploads/ddb44051ba9e6a5ccd262e8e9560c7c8/스크린샷_2020-12-17_오후_10.13.01.png)

<br/><br/>

### 1-2. Log 읽고 쓰기
```
**설명**

log들은 init_db의 log_path 인자로 들어오는 파일에 저장되고
log msg들은 init_db의 logmsg_path 인자로 들어오는 파일에 저장됩니다.

log는 실행하면 실행 할수록 계속 이어쓰는 구조를 택했고
log msg는 실행 할 때마다 초기화 되어 처음부터 쓰게 구현하였습니다.

log record들은 (Begin, Commit, Rollback), Update, Compensate가 있습니다.
이들은 각각 28byte, 288byte, 296byte의 형식을 갖습니다.
(구조체 패딩으로 인해 자동으로 32, 296, 304byte를 갖기 때문에
#pragma pack(1)와 #pragma pack(8)을 사용해 size를 맞춰주었습니다.)

Begin - trx가 begin(시작)할때 발급합니다.
Commit - trx가 commit 할때 발급합니다.
Rollback - trx가 정상적으로 abort 되어 rollback을 마쳤을때 발급합니다.
Update - trx가 db_update를 통해 update를 진행하였을때 발급합니다.
Compensate - trx가 rollback 될때 각각의 update에 대해 발급합니다.
    (trx_abort 될 때나 deadlock에 의해 roll_back 될 때,
    정상적으로 종료하지 못한 update trx를 undo phase에서 처리할 때)

LSN으로는 log file에서의 시작 offset을 사용하였습니다.

compensate의 old_img에는 compensate에 해당하는 update의 new_img를
compensate의 new_img에는 compensate에 해당하는 update의 old_img를
담았습니다.
이유는 redo 시 update와 일괄적으로 처리하기 위해서 입니다.


또한 기존에 사용하던 buff가 아닌 새로운 log의 buff를 만들었습니다.
log의 buff은 기존의 buff보다 1만큼 크게 초기화를 했으며
log들은 buff block에 순차적으로 적어지게 됩니다.

또한 log buff의 마지막 block은 읽기 전용으로 만들어
file에서 log를 불러와야 할때 불러와서 저장하는 구조를 갖게 만들었습니다.

해당 log buff는 page eviction, trx_commit, trx_abort가 일어날때
disk로 flush 됩니다.


***API는 2장에서 설명하겠습니다.***
```

<br/><br/>

### 1-3. Recovery

```
**설명**

Recovery는 3단계로 이루어집니다.

Anaylsis phase
    trx가 winner인지 loser인지 구별합니다.

    제 구현에선 log 파일을 처음부터 끝까지 읽으며 
    recoveryManager에 winner trx, loser trx entry를 각각 만듭니다.

    또한 각각의 trx 마다 last_LSN(trx이 마지막으로 적은 log의 LSN)를 관리함으로써
    이후 compensate log의 
    LSN이나 prev_LSN, next_undo_LSN 등을 손쉽게 설정할 수 있게됩니다.



Redo phase
    log 파일을 다시 처음부터 끝까지 읽으며
    Winner와 Loser와 관계없이 모든 log들을 
    recoveryManager의 redo entry에 linked list로 연결합니다.

    이후 init_db의 flag와 log_num 값에 맞게
    log들을 순차적으로 redo 시킵니다.

    만약 page의 LSN이 log의 LSN보다 크다면
    최신 page로 간주하고 consider redo 합니다.



Undo phase
    앞서 Anaylsis에서 구별한 loser trx들의 
    last_LSN(해당 trx가 발급한 마지막 log의 LSN) 을 이용해서 
    loser trx들의 마지막 log들을 불러옵니다.

    이후 이 log들을 묶은 linked list를 만드는데
    log들은 LSN의 내림차순으로 정렬되어 있어
    undo 시 순서에 의한 오작동을 방지합니다.

    이후 list의 맨 앞의 log를 꺼낸 후
    log가 update log면 compensation log를 발급하고
    log가 compensation log이면 next_nudo_LSN을 보고
    다음 undo 할 곳을 찾아 compensate log를 발급한 후 undo 합니다.

***API는 2장에서 설명하겠습니다.***
```


<br/><br/>

------

## 2. API

### 2-1. General Lock
```
기존에 page latch를 풀고 lock_acquire를 부르는 방식이 아닌
page latch를 잡고 lock_acquire를 부르는 경우가 생겼으므로

lock_acquire 함수를 변경하였습니다.

project5의 lock_acquire와 다른 점은 lockt ** ret_lock을 parameter로 받으므로서
return 값을 세분화 하여 다양한 경우의 수를 생각할 수 있게 되었습니다.

return 값이 -1 이면 비정상적인 종료를
return 값이 0 이면 sleep 할 필요가 없음을
return 값이 1 이면 sleep 해야 함을
return 값이 2 이면 deadlock 상황을 의미합니다.

이를 활용하여 db_find, db_update에서 다양항 상황에
적절하게 대처할 수 있게 하였습니다.


lock_release 함수는 거의 변한 점이 없지만
달라진 점은 해당 obj에 signal을 보내기 전에
해당 obj의 주인 trx의 latch를 획득해야 한다는 점입니다.

이로 인해 general lock으로 변경되며 발생할 수 있었던
lost wake up problem을 해결할 수 있게 되었습니다.


```c
int
lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, char * value, lock_t ** ret_lock)
``` 

<br/><br/>

<br/><br/>

### 2-2. Log 읽고 쓰기
```
Log를 읽고 쓰는 과정과 API에 대한 설명입니다.
Log는 file에 저장되어 있고 Log를 쓰는 곳은 상위 레이어이기 때문에
각 계층별로 함수들을 구현해주어야 할 필요성이 있습니다.


file layer에선 
log_file_open
log_file_read_record
log_file_write_record
함수들을 통해 log 파일을 열고, 읽고 쓰기를 진행합니다.


buffer layer에선
init_log_buff
log_buff_find
print_log_buff
log_buff_flush
log_buff_search_record
log_buff_write
log_buff_read_all_file
함수들을 이용해 buffer에 log를 올리고 buffer에서 log를 내리는 일을 합니다.

최상위 layer에선
buffer를 이용해 실제 log를 작성하거나
recovery를 수행하게 됩니다.
``` 

```c
int log_file_open(int * file, char * pathname, log_record_t * last_record, char * logmsg_path);
void log_file_write_record(int file, int start_point, int type, const log_record_t* src);
int log_file_read_record(int file, int start_point, int type, const log_record_t* dest);


int init_log_buff(int num_buf, char * log_path, char * logmsg_path);
int log_buff_find();
void print_log_buff(void);
void log_buff_flush();
int log_buff_search_record(int file, int tar_LSN, const log_record_t* dest);
void log_buff_write(int64_t trx_prev_LSN, int trx_id, int type, int table_id,pagenum_t page_NUM, int index, char * old_img, char * new_img);
void log_buff_read_all_file(void);
```
<br/><br/>

### 2-3. Recovery
```
제가 구현한 recovery는 recovery manager에 의해 수행됩니다.

r_trx를 이용해 recovery에서 관리한 trx들을 linked list로 만들고
re_entry에는 redo phase에서 적용될 redo log들이 linked list를 이룹니다.
un_entry에는 undo phase에서 적용될 undo log들이 list를 이룹니다.
    (LSN이 큰 entry가 head에 가깝게 있다.
        따라서 head의 entry를 뽑게 되면 가장 먼저 undo 해야할 log를 찾을 수 있다.)

init_recovM으로 recoveryManager를 초기화하고
trx_begin log가 나오면 r_trx_begin 함수로 manager에 해당 trx를 추가합니다.
r_trx_find는 manager에서 해당 trx를 찾아주는 함수입니다.


recov_analysis 함수를 통해 analysis phase가 진행됩니다.
who_is_winner 와who_is_loser 함수로 winner trx를 나눕니다.


log 파일을 처음부터 읽으며
add_redo_entry 함수로 redo 할 log를 추가해줍니다.
recov_redo 함수를 통해 실제 redo가 진행됩니다.


recov_undo 함수를 통해 실제 undo가 진행됩니다.
add_undo_entry는 manager의 undo_entry에 entry를 추가하는 함수인데
추가시 entry는 LSN을 기준으로 정렬된 상태를 유지합니다.
del_undo_entry 함수로 undo_entry 중 LSN이 가장 큰 log를 뽑고
현재 undo를 진행해야 할 log를 찾아야 한다면 찾아주고
undo를 진행합니다.



``` 

```c
#define WINNER_TRX 1
#define LOSER_TRX 0

typedef struct r_trx{
        struct r_trx * next;
        int id;
        int type;
        int64_t last_LSN;
}r_trx;

typedef struct re_entry{
        struct re_entry * next;
        log_record_t content;
}re_entry;

typedef struct un_entry{
        struct un_entry * prev;
        struct un_entry * next;
        log_record_t content;
}un_entry;

typedef struct recoveryManager{
        r_trx * head;
        int64_t log_cnt;

        re_entry * redo_head;
        un_entry * undo_head;

        int64_t winner_cnt;
        int * winner_entry;

        int64_t loser_cnt;
        int * loser_entry;
}recoveryManager;

void init_recoveryM(void);
r_trx * r_trx_begin(int trx_id);
r_trx * r_trx_find(int trx_id);
void print_recovM(void);

int * who_is_winner(void);
int * who_is_loser(void);
void recov_analysis(void);

void add_redo_entry(log_record_t * content);
void recov_redo(int flag, int log_num);

void add_undo_entry(log_record_t * content);
un_entry * del_undo_entry(void);
void recov_undo(int flag, int log_num);
```


<br/><br/>

------


## 3. Test Result

### 4-1. 상황1
```
b == begin, u == update, c == commit, com == compensate, r == rollback
makelogfile 폴더의 main을 조작하고
실행파일을 실행시켜 
logfile을 생성 시킨 후 테스트 하였습니다.

모든 테스트는 leaf order == 4, internal order == 3에서 진행되었습니다.
```


```
id 
1  b u       u   u c
2      b u u   u       com com com r
3                    b                   u          com r
4                                     b    u  com r
``` 

![스크린샷_2020-12-17_오후_11.14.24](uploads/e17d164d9406991619aab7ef8c430e76/스크린샷_2020-12-17_오후_11.14.24.png)

### 4-1. 상황2
```
b == begin, u == update, c == commit, com == compensate, r == rollback
id 
1  b u       u   u 
2      b u u   u      
3                   b      u          
4                       b    u  
``` 

![스크린샷_2020-12-17_오후_11.15.03](uploads/130c603f09ff43bdf38c212d857a5964/스크린샷_2020-12-17_오후_11.15.03.png)

### 4-3. flag 1
```
init_db의 flag에 1을 log_num에 3을 준 상황입니다.
analysis phase는 종료하고 3개의 log에 대해서만 redo 를 진행한 후 종료되는 모습입니다.
``` 

![스크린샷_2020-12-17_오후_11.16.27](uploads/46f33d36b61f3b2eaeb0b34a95cb2ea6/스크린샷_2020-12-17_오후_11.16.27.png)

### 4-4. flag 2
```
init_db의 flag에 2를 log_num에 3을 준 상황입니다.
redo phase는 끝나고 3개의 log에 대해서만 undo 를 진행한 후 종료하는 모습입니다.
``` 

![스크린샷_2020-12-17_오후_11.18.08](uploads/21c69a281ad6166edb83cc2407524871/스크린샷_2020-12-17_오후_11.18.08.png)

<br/><br/>

---

## 4. 문제점
```
이번 프로젝트 역시 이전 프로젝트와 마찬가지로
기능 하나 하나를 구현하고 unittest를 진행 한 후
각각의 기능들을 합쳐 나가면서 구현하였습니다.

1. chang_open_table, open_table 변경

2. generalLock, general lock 구현 후 싱글스레드에서 동작 하는지

3, generalLock_multithread, general lock 구현 후 멀티스레드에서 동작 하는지

4. log_manager, log를 읽고 쓰는 일이 가능한지

5-1. generalLock&log,  general lock과 log manager를 합쳤을때 제대로 동작하는지
5-2. generalLock&log&roll&abort
    compensate log 발급을 위해 roll_back과 trx_abort를 나누고 수정함

6. recovery_manager, recoveryManager를 구현하고 3-pass recovery 실험

7. lock&log&recovery, 최종 결과물을 합치고 test

이후 실행 결과까지 확인했지만
MAC OS와는 다르게 LINUX OS에서는 log file을 정상적으로 읽어오지 못하는 것을 확인하여
고치려고 노력했으나 성공하지 못 하였습니다.

해당 파일들은 project6의 unittest 폴더 안에 첨부하였습니다.
``` 