#ifndef __BPT_H__
#define __BPT_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


typedef uint64_t pagenum_t;

//for print tree
void enqueue( pagenum_t pNum );
pagenum_t dequeue( void );
void print_tree();
void print_tree2();

//for insert
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

//for delete
pagenum_t delete_entry( pagenum_t nN, int64_t key );
pagenum_t adjust_root(pagenum_t rootN);
int get_neighbor_index( pagenum_t nN );
pagenum_t coalesce_nodes( pagenum_t nN, pagenum_t neighborN, int neighbor_index, int64_t k_prime);
pagenum_t redistribute_nodes( pagenum_t nN, pagenum_t neighborN, int neighbor_index, 
        int k_prime_index, int64_t k_prime);


int open_table(char* pathname);
int db_insert(int64_t key, char * value);
int db_find(int64_t key, char * ret_val);
int db_delete(int64_t key);

#endif /* __BPT_H__*/
