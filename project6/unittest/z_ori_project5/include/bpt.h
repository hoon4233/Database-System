#include "buffer.h"
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdbool.h>

//for print tree
void enqueue( pagenum_t pNum );
pagenum_t dequeue( void );
void print_tree(int table_id);
void print_tree2(int table_id);

//for insert
pagenum_t make_internal( int table_id );
pagenum_t make_leaf( int table_id );
void start_new_tree(int table_id, int64_t key, char * value);
pagenum_t find_leaf(int table_id, int64_t key);
pagenum_t insert_into_leaf( int table_id, pagenum_t leaf_pageN, int64_t key, char * value );
pagenum_t insert_into_leaf_after_splitting( int table_id, pagenum_t leaf_pageN, int64_t key,
                                        char * value);

int cut( int length );
pagenum_t insert_into_parent( int table_id, pagenum_t leftN, int64_t key, pagenum_t rightN);
pagenum_t insert_into_new_root(int table_id, pagenum_t leftN, int64_t key, pagenum_t rightN);

int get_left_index(int table_id, pagenum_t parentN, pagenum_t leftN);

pagenum_t insert_into_node( int table_id, pagenum_t nN, 
        int left_index, int64_t key, pagenum_t rightN);
pagenum_t insert_into_node_after_splitting( int table_id, pagenum_t old_pageN, int left_index, 
        int64_t key, pagenum_t rightN);

//for delete
pagenum_t delete_entry( int table_id, pagenum_t nN, int64_t key );
pagenum_t adjust_root(int table_id, pagenum_t rootN);
int get_neighbor_index( int table_id, pagenum_t nN );
pagenum_t coalesce_nodes( int table_id, pagenum_t nN, pagenum_t neighborN, int neighbor_index, int64_t k_prime);
pagenum_t redistribute_nodes( int table_id, pagenum_t nN, pagenum_t neighborN, int neighbor_index, 
        int k_prime_index, int64_t k_prime);


int open_table(char* pathname);
int db_insert(int table_id, int64_t key, char * value);
int db_find(int table_id, int64_t key, char * ret_val, int trx_id);
int db_delete(int table_id, int64_t key);
int close_table(int table_id);

int init_db(int num_buf);
int shutdown_db();

int db_update(int table_id, int64_t key, char * values, int trx_id);


//project5

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

struct lock_t {
	lock_t * prev;
	lock_t * next;
	lock_e * sentinel;
	pthread_cond_t cv;
	int mode;
	lock_t * trxnextlock;
	lock_t * trxprevlock;
	int owner_id;

    int tid;
    int kid;
    char old_v[VALUE_SIZE];
};

//trxM
typedef struct trx{
    int id;
    lock_t * first_obj;
    struct trx * prev;
    struct trx * next;
}trx;

typedef struct trxManager{
    int global_trx_id;
    trx * head_trx;
}trxManager;


//deadlock
typedef struct Gnode{
    int vertex;
    int * edge;
    int used_cnt;

    struct Gnode * next;
    struct Gnode * prev;
}Gnode;

typedef struct graphManager{
    Gnode * head_node;
    Gnode * tail_node;
}graphManager;

typedef struct visit{
    int vertex;
    int is_check;

    struct visit * next;
    struct visit * prev;
}visit;

typedef struct visitManager{
    visit * head;
    visit * tail;
}visitManager;



//lock table
int init_lock_table(void);
lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, char * value);
int lock_release(lock_t* lock_obj);
void print_hash_table(void);
Node* hash_find(int table_id, int64_t record_id);
Node *  hash_insert(int table_id, int64_t record_id);


//trxM
void init_trxM(void);
void print_trxM(void);
trx * trx_find(int trx_id);
int trx_begin(void);
int trx_commit(int trx_id);
int add_obj_trx(int trx_id, lock_t * lock_obj);
int roll_back(int trx_id);
int trx_abort(int trx_id);


//deadlock
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