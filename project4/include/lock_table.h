#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <stdint.h>

typedef struct lock_t lock_t;

typedef struct lock_e{
	int tid;
	int64_t rid;
	lock_t * head;
	lock_t * tail;
}lock_e;

typedef struct Node
{
	lock_e * lock_entry;
	struct Node * next;
}Node;

/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int table_id, int64_t key);
int lock_release(lock_t* lock_obj);
void print_hash_table();
Node* hash_find(int table_id, int64_t record_id);
Node *  hash_insert(int table_id, int64_t record_id);

#endif /* __LOCK_TABLE_H__ */
