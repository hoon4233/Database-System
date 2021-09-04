# Project4 - Lock Table
## 목차
1. 디자인
   1. Lock Table
   2. Node & hash table
   3. lock_e & lock_t
2. 동작 설명
   1. init_lock_table
   2. lock_acquire
   3. lock_release
3. 테스트 케이스
   1. Table_num = 3, Record_num = 5
   2. Table_num = 12, Record_num = 5
   3. Table_num = 3, Record_num = 12
   4. Table_num = 12, Record_num = 12
   5. Transfer thread = 5, Scan thread = 3
   6. Transfer thread = 11, Scan thread = 1

------

## 1. 디자인

### 1-1. Lock Table
```
**설명**
이번 프로젝트에 쓰일 Lock Table을 설명합니다.

해당 그림은 lock table에 entry가 생성되어 있고
entry 마다 lock object가 생성되어 있는 예시입니다.

***entry와 object가 추가&삭제되는 과정은 2장에서 설명하겠습니다.***
``` 

![4-1](https://user-images.githubusercontent.com/26400022/132084491-0aa824a2-4abb-4f61-a9ff-cd96358a8f40.jpeg)

<br/><br/>

### 1-2. Node & hash table

```c
//Nodes
typedef struct Node
{
	lock_e * lock_entry;
	struct Node * next;
}Node;
```

```
**Node의 structure**

Node에서는 hash table에서 
동일한 key 값을 갖는 entry들을 (next 변수로) 엮어줍니다.

이 Node들이 모여 완전한 hash table을 만들게 됩니다.

hash table은 hash_key, hash_find, hash_insert로 동작합니다.

***구체적인 활용은 2장에서 설명하겠습니다.***
```

```c
int hash_key(int table_id, int64_t record_id)
```
hash_key 함수는 table_id와 record_id를 인자로 받아
hash key 값을 계산해줍니다.<br/><br/>
제가 구현한 방법은 table_id와 record_id를 concat 시킨 후
concat 시킨 값을 hash table의 size로 나눈 값을
key로 사용합니다.
<br/><br/>

```c
Node* hash_find(int table_id, int64_t record_id)
```
hash_find 함수는 table_id와 record_id를 인자로 받아<br/><br/>
해당하는 lock_entry를 hash table에서 찾아줍니다.
<br/><br/>

```c
Node * hash_insert(int table_id, int64_t record_id)
```
hash_insert 함수는 table_id와 record_id를 인자로 받아<br/><br/>
해당하는 lock_entry를 hash table에 넣어줍니다.<br/><br/>
해당 함수를 호출하기 전에 hash_find로<br/><br/>
넣고자하는 entry가 있는지 확인하고 호출하기 때문에<br/><br/>
해당 entry가 있는 경우에는 절대 호출할 일이 없습니다.
<br/><br/>


<br/><br/>

### 1-3. lock_e & lock_t

```c
typedef struct lock_e{
	int tid;
	int64_t rid;
	lock_t * head;
	lock_t * tail;
}lock_e;
```

```c
struct lock_t {
	lock_t * prev;
	lock_t * next;
	lock_e * sentinel;
	pthread_cond_t cv;
};
```

```
**lock_e의 structure**
lock entry의 structure입니다.

entry들은 서로를 구별하기 위해 
table_id(tid), record_id(rid)를 갖습니다.

head와 tail은 해당 entry의 object들을 관리합니다.

add_obj로 entry에 object를 추가 할 수 있고
del_obj로 entry에서 object를 삭제할 수 있습니다.



**lock_t의 structure**
lock object의 structure입니다.

object들은 linked list로 구성되어 있기 때문에
prev와 next 포인터를 갖습니다.

sentinel은 object를 관리하는 entry를 가리키며
object 삭제를 손쉽게 만들어줍니다.

cv는 해당 object의 conditional variable로서
이전 object가 release 되면서 
해당 object를 깨워주기 위해 사용합니다.

***구체적인 활용은 2장에서 설명하겠습니다.***
```

```c
lock_t * add_obj(lock_e * entry)
```
add_obj 함수는 인자로 받은 entry에 object 개를 추가합니다<br/><br/>
object 1개를 초기화 해주고 entry가 관리하는 list의 마지막에 추가합니다.
<br/><br/>

```c
lock_t * del_obj(lock_t * obj)
```
del_obj 함수는 인자로 받은 object를 entry에서 삭제합니다.<br/><br/>
object가 삭제되는 경우는<br/><br/>
entry가 관리하는 list의 가장 처음일때 밖에 없습니다.
<br/><br/>

<br/><br/>

------

## 2. 동작 설명

### 2-1. init_lock_table
```
lock table을 초기화 합니다.
특별한 동작없이 table이 모두 NULL을 가리키게 해줍니다.
``` 

```c
int
init_lock_table()
{
	for(int i = 0; i<HASH_SIZE; i++){
		hash_table[i] = NULL;
	}

	return 0;
}
```
<br/><br/>

### 2-2. lock_acquire
```
table_id와 key를 인자로 받아
해당 entry에 대한 lock을 획득하기 위해 호출하는 함수입니다.

일단 hash_find로 hash table에 해당 entry가 있는지 확인합니다.
    만약 없다면 hash_insert로 해당 entry를 만들어줍니다.

이후 add_obj함수로 entry에 object를 추가해줍니다.

이후 추가한 object가 entry의 첫번째 object인지 확인하고
그렇지 않다면 이전 object가 일을 마치고 깨워주길 기다립니다.
``` 

```c
lock_t*
lock_acquire(int table_id, int64_t key)
{
	pthread_mutex_lock(&lock_table_latch);
	lock_t * obj = NULL;
	Node * node = NULL;

	node = hash_find(table_id, key);
	
	if(node == NULL){
		count += 1; //for debug
		node = hash_insert(table_id, key);
	}
	
	obj = add_obj(node->lock_entry);

	if(obj->prev != NULL ){
		pthread_cond_wait(&obj->cv,&lock_table_latch);
	}

	pthread_mutex_unlock(&lock_table_latch);

	if(obj != NULL){
		return obj;
	}

	return (void*) 0;
}
```
<br/><br/>

### 2-3. lock release
```
인자로 object를 받아 해당 object를 삭제해줌으로서
해당 entry의 lock을 반납하는 함수입니다.

먼저 del_obj로 entry에서 해당 obj를 삭제해줍니다.
    삭제되는 object는 무조건 entry의 첫번째 object입니다.

이후 기다리고 있는 object가 있다면 
그 object에 signal을 주어 깨워줍니다.
``` 

```c
int
lock_release(lock_t* lock_obj)
{
	pthread_mutex_lock(&lock_table_latch);
	lock_t * next_obj = NULL;

	next_obj = del_obj(lock_obj);
	
	if(next_obj != NULL){
		pthread_cond_signal(&next_obj->cv);
	}
	pthread_mutex_unlock(&lock_table_latch);
	return 0;
}
```

<br/><br/>

------

## 3. 테스트 케이스
```
모든 test case는
hash table size = 10에서 실행되었습니다.

이는 test case의 단순화를 위함이고
실제 제출은 table size = 5000으로 제출하였습니다.
```

### 3-1. Table_num = 3, Record_num = 5
```
table의 수와 record의 수가
모두 hash table의 size보다 작을때 입니다.
``` 

<img width="174" alt="4-2" src="https://user-images.githubusercontent.com/26400022/132084494-d8407afd-b327-42c2-81c4-296bad54c2ca.png">


### 3-2. Table_num = 12, Record_num = 5
```
table의 수는 hash table의 size보다 크고
record의 수는 hash table의 size보다 작을때 입니다.
``` 

<img width="604" alt="4-3" src="https://user-images.githubusercontent.com/26400022/132084495-d767a502-1bd0-4733-82a5-153e513d48f9.png">

### 3-3. Table_num = 3, Record_num = 12
```
table의 수는 hash table의 size보다 작고
record의 수는 hash table의 size보다 클때 입니다.
``` 

<img width="345" alt="4-4" src="https://user-images.githubusercontent.com/26400022/132084496-17c3ba11-d58a-4c43-ad74-9627f2d05a41.png">

### 3-4. Table_num = 12, Record_num = 12
```
table의 수와 record의 수가
모두 hash table의 size보다 클때 입니다.
``` 

<img width="1031" alt="4-5" src="https://user-images.githubusercontent.com/26400022/132084497-60bd7e19-79b0-4393-bddc-c8ccb25ff4ca.png">

### 3-5. Transfer thread = 5, Scan thread = 3
```
Transfer thread는 5로
Scanf thread는 3으로 했을때의 결과입니다.
Table num은 3이고 Record num은 5입니다.
``` 

<img width="189" alt="4-6" src="https://user-images.githubusercontent.com/26400022/132084498-47d37757-b052-4703-b7df-e4947c7c8045.png">

### 3-6. Transfer thread = 11, Scan thread = 1
```
Transfer thread는 11로
Scanf thread는 1으로 했을때의 결과입니다.
Table num은 3이고 Record num은 5입니다.
``` 

<img width="171" alt="4-7" src="https://user-images.githubusercontent.com/26400022/132084499-59af869e-6304-4870-8889-972ee5a7ad87.png">

