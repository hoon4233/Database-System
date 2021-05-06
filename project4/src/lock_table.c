#include <lock_table.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>

#define HASH_SIZE 5000

int count = 0;

typedef struct lock_e lock_e;

struct lock_t {
	lock_t * prev;
	lock_t * next;
	lock_e * sentinel;
	pthread_cond_t cv;
};

typedef struct lock_t lock_t;

// typedef struct lock_e{
// 	int tid;
// 	int64_t rid;
// 	lock_t * head;
// 	lock_t * tail;
// }lock_e;

// typedef struct Node
// {
// 	lock_e * lock_entry;
// 	struct Node * next;
// }Node;


Node * hash_table[HASH_SIZE];
pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;


///////about hash
void print_hash_table()
{
	printf("\nPrint All hash data \n");
    for (int i = 0; i < HASH_SIZE; i++)
    {
        if (hash_table[i] != NULL)
        {
			printf("idx : %d \n",i);
			
            Node* node = hash_table[i];
            while (node->next)
            {
				// printf("%d, %lld //", node->lock_entry->tid, node->lock_entry->rid);
				lock_t * tmp = node->lock_entry->head;
				int count = 0;
				while(tmp){
					count += 1;
					tmp = tmp->next;
				}
				printf("%d, %lld // ", node->lock_entry->tid, node->lock_entry->rid);
				// printf("%d, %lld, count : %d // ", node->lock_entry->tid, node->lock_entry->rid,count);
                node = node->next;
            }
			lock_t * tmp = node->lock_entry->head;
			int count = 0;
			while(tmp){
				count += 1;
				tmp = tmp->next;
			}
			printf("%d, %lld // \n", node->lock_entry->tid, node->lock_entry->rid);
            // printf("%d, %lld, count : %d\n", node->lock_entry->tid, node->lock_entry->rid, count);
        }
    }
    printf("\n\n");
}

int hash_key(int table_id, int64_t record_id){
	char tid_s[50];  //int 최대값 2147483647, rid_s를 뒤에 붙일거니까
	char rid_s[25];  //int 64_t 최대값 9223372036854775807
	int ret = 0;

	sprintf(tid_s,"%d",table_id);
	sprintf(rid_s,"%lld",record_id);
	strcat(tid_s,rid_s);

	ret = atoi(tid_s)%HASH_SIZE;
	// ret = (table_id + record_id) % HASH_SIZE;

	return ret;
}

Node* hash_find(int table_id, int64_t record_id)
{
    int h_key = hash_key(table_id, record_id);
	
	if(h_key < 0){
		return NULL;
	}
    if (hash_table[h_key] == NULL)
        return NULL;

	Node* tmp = hash_table[h_key];
 
    if (tmp->lock_entry->tid == table_id && tmp->lock_entry->rid == record_id )
    {
        return tmp;
    }
    else
    {
        while (tmp->next)
        {
            
			if (tmp->next->lock_entry->tid == table_id && tmp->next->lock_entry->rid == record_id)
            {
                return tmp->next;
            }
            tmp = tmp->next;
        }
    }
    return  NULL;
}

Node * hash_insert(int table_id, int64_t record_id)
{
	Node * tmp_node = (Node *)malloc(sizeof(Node));
	tmp_node->lock_entry = (lock_e *)malloc(sizeof(lock_e));
	tmp_node->lock_entry->tid  = table_id;
	tmp_node->lock_entry->rid  = record_id;
	tmp_node->lock_entry->head  = NULL;
	tmp_node->lock_entry->tail  = NULL;
	tmp_node->next = NULL;

    int h_key = hash_key(table_id, record_id);
    if (hash_table[h_key] == NULL) //collision 안나면 그냥 넣어줌
    {
        hash_table[h_key] = tmp_node;
		// return tmp_node;
    }
    else //collision 나면 앞에 넣어줌
    {
		if(hash_find(table_id, record_id) == NULL){
			tmp_node->next = hash_table[h_key];
			hash_table[h_key] = tmp_node;
			// return tmp_node;
		}
    }
	return tmp_node;
}


////////about obj
lock_t * add_obj(lock_e * entry){
	lock_t * tmp = (lock_t *)malloc(sizeof(lock_t));
	tmp->prev = NULL;
	tmp->next = NULL;
	tmp->sentinel = entry;
	
	int ret = pthread_cond_init(&tmp->cv,0);
	

	if(entry->head == NULL){
		entry->head = tmp;
		entry->tail = tmp;
		return tmp;
	}

	lock_t * ori_tail = entry->head;
	while(ori_tail->next){
		ori_tail = ori_tail->next;
	}
	ori_tail->next = tmp;
	tmp->prev = ori_tail;
	entry->tail = tmp;
	return tmp;
}

lock_t * del_obj(lock_t * obj){
	lock_e * entry = obj->sentinel;
	lock_t * tmp;
	entry->head = obj->next;
	if(obj->next == NULL){
		entry->tail = NULL;
		free(obj);
		return NULL;
	}
	tmp = obj->next;
	tmp->prev = NULL;
	free(obj);
	return tmp;
}





//////about lock
int
init_lock_table()
{
	for(int i = 0; i<HASH_SIZE; i++){
		hash_table[i] = NULL;
	}

	return 0;
}

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
