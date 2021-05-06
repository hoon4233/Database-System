#include "bpt.h"

extern buffer * buff;
extern tManager * tableM;
extern page_t header;

#define FIND_THREAD (0)
#define UPDATE_THREAD (1)

#define INSERT_NUM (10)
#define FIND_NUM (10)
#define UPDATE_NUM (10)

int count = -1;
int count2 = -2;
pthread_mutex_t count_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t count2_latch = PTHREAD_MUTEX_INITIALIZER;

void*
find_thread_func(void* arg)
{
    sleep(2);
    int id = pthread_self();
	char val[VALUE_SIZE];
    int tmp_count;
    int trid = trx_begin();

    printf("new find Thread id : %d, trx id : %d\n",id, trid);

    pthread_mutex_lock(&count_latch);
    count += 1;
    tmp_count = count;
    pthread_mutex_unlock(&count_latch);
    
    int ret = db_find(1,tmp_count, val, trid);
    // int ret = db_find(1,3, val, trid);

    if(ret == 1){
        printf("find thread id : %d, trx id : %d // db_find fail and trx_abort\n",id, trid);
        return NULL;
    }
    if(ret == 2){
        printf("find thread id : %d, trx id : %d // roll back\n",id, trid);
        return NULL;
    }
    
    // printf("find thread id : %d, trx id : %d, result : %s\n",id, trid, val);
    // printf("find end and acq s lock about key 3\n");
    // sleep(2);

    // ret = db_find(1,2, val, trid);
    // if(ret == 1){
    //     printf("find thread id : %d, trx id : %d // db_find fail and trx_abort\n",id, trid);
    //     return NULL;
    // }
    // if(ret == 2){
    //     printf("find thread id : %d, trx id : %d // roll back\n",id, trid);
    //     return NULL;
    // }

    ret = trx_commit(trid);
    if(ret == 0 ){
        printf("find thread id : %d, trx id : %d // trx_commit fail\n",id, trid);
        return NULL;
    }
    printf("second thread id : %d, trx id : %d, result : %s\n",id, trid, val);

	return NULL;
}

void*
update_thread_func(void* arg)
{
    // sleep(1);
    int id = pthread_self();
	char val[VALUE_SIZE];
    int tmp_count;
    int trid = trx_begin();

    printf("new update Thread id : %d, trx id : %d\n",id, trid);

    pthread_mutex_lock(&count2_latch);
    count2 += 1;
    tmp_count = count2;
    pthread_mutex_unlock(&count2_latch);
    

    sprintf(val,"%d",tmp_count);
    int ret = db_update(1,tmp_count+1, val, trid);
    // sprintf(val,"%d",-1);
    // int ret = db_update(1,2, val, trid);

    if(ret == 1){
        printf("update thread id : %d, trx id : %d, key : %d // db_update fail and trx_abort\n",id,trid,tmp_count+1);
        return NULL;
    }
    if(ret == 2){
        printf("update thread id : %d, trx id : %d, key : %d // roll_back\n",id,trid,2);
        return NULL;
    }
    
    // printf("update thread id : %d, trx id : %d, key : %d. // succ\n",id,trid,2);
    // printf("update end and acq x lock about key 2\n");
    // sleep(3);

    // sprintf(val,"%d",-1);
    // ret = db_update(1,3, val, trid);
    // if(ret == 1){
    //     printf("update thread id : %d, trx id : %d, key : %d // db_update fail and trx_abort\n",id,trid,3);
    //     return NULL;
    // }
    // if(ret == 2){
    //     printf("update thread id : %d, trx id : %d, key : %d // roll_back\n",id,trid,2);
    //     return NULL;
    // }

    // if(ret == 0 ){
    //     printf("update thread id : %d, trx id : %d // trx_commit fail\n",id, trid);
    //     return NULL;
    // }
    printf("first thread id : %d, trx id : %d, key : %d. // succ\n",id,trid,tmp_count+1);
    // sleep(5);
    ret = db_find(1,tmp_count+1, val, trid);
    if(ret == 0){
        printf("fisrt thread id : %d, trx id : %d, result : %s\n",id, trid, val);
    }
    sleep(10);
    

    ret = trx_commit(trid);
	return NULL;
}

int main(int argc, char ** argv){
    int ret = init_db(50);
    int tableNum = open_table(argv[1]);

    int table_id = 1;
    int key;
    int value[VALUE_SIZE];
    int tmp_id = trx_begin();
    for(int i =0; i<INSERT_NUM; i++){
        key = i;
        sprintf(value,"%d",i);
        // printf("IN main, i : %d\n",i);
        ret = db_insert(table_id, key, value);
        // print_tree(1);
    }
    tmp_id = trx_commit(1);

    pthread_t find_thread[FIND_THREAD];
    pthread_t update_thread[UPDATE_THREAD];

    print_tree(1);
    // sleep(5);

    for(int i = 0; i<FIND_THREAD; i++){
        pthread_create(&find_thread[i],0,find_thread_func,NULL);
    }
    for(int i = 0; i<UPDATE_THREAD; i++){
        pthread_create(&update_thread[i],0,update_thread_func,NULL);
    }

    for(int i = 0; i<FIND_THREAD; i++){
        pthread_join(find_thread[i],NULL);
    }
    for(int i = 0; i<UPDATE_THREAD; i++){
        pthread_join(update_thread[i],NULL);
    }
    // printf("count : %d\n",count);
    print_tree(1);

    return 0;
    
}

// int main( int argc, char ** argv ) {
//     char instruction;
//     int64_t key;
//     char value[VALUE_SIZE];
//     int table_id;

//     int ret_int;
//     int ret;
//     int tableNum;
    
//     if(argc >= 2){
//         ret = init_db(100);
//         tableNum = open_table(argv[1]);
//     }
//     else{
//         ret = init_db(1000);
//         tableNum = open_table("sample.db");
//     }
    
//     table_id = 1;
//     for(int i =0; i<10; i++){
//         key = i;
//         sprintf(value,"%d",i);
//         ret = db_insert(table_id, key, value);
//         print_tree(table_id);
//     }
    

//     printf("> ");
//     while (scanf("%c", &instruction) != EOF) {
//         switch (instruction) {
//         case 'd':
//             scanf("%d %lldd", &table_id, &key);
//             ret_int = db_delete(table_id,key);
//             print_tree(table_id);
//             break;
//         case 'i': 
//             scanf("%d %lld %s", &table_id, &key, value);
//             ret_int = db_insert(table_id,key, value);
//             print_tree(table_id);
//             break;
//         case 'f':
//             scanf("%d %lld", &table_id, &key);
//             ret_int = db_find(table_id,key, value);
//             printf("find result : %s\n", value);
//             break;
        
//         case 'q':
//             while (getchar() != (int)'\n');
//             for(int i=1; i<10; i++){
//                 close_table(i);
//             }
//             shutdown_db();
//             return EXIT_SUCCESS;
//             break;

//         case 't':
//             scanf("%d", &table_id);
//             print_tree(table_id);
//             break;

//         case 'u':
//             scanf("%d %lld %s", &table_id, &key, value);
//             ret_int = db_update(table_id,key, value);
//             break;
            
//         default:
//             break;
//         }
//         while (getchar() != (int)'\n');
//         printf("> ");
//     }
//     printf("\n");
//     return EXIT_SUCCESS;
// }
