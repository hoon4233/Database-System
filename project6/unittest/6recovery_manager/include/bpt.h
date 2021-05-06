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
int db_find(int table_id, int64_t key, char * ret_val);
int db_delete(int table_id, int64_t key);
int close_table(int table_id);

int init_db(int num_buf, char * log_path);
int shutdown_db();



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

        int64_t loser_cnt;
        int * loser_entry;
}recoveryManager;

void init_recoveryM(void);
r_trx * r_trx_begin(int trx_id);
r_trx * r_trx_find(int trx_id);
void print_recovM(void);

int * who_is_loser(void);
void recov_analysis(char * log_path, char * logmsg_path);

void add_redo_entry(log_record_t * content);
void recov_redo(char * log_path, char * logmsg_path);

void add_undo_entry(log_record_t * content);
un_entry * del_undo_entry(void);
void recov_undo(char * log_path, char * logmsg_path);