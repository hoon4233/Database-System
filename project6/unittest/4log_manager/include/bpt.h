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