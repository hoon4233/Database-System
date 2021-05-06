#include "bpt.h"

extern buffer * buff;
extern tManager * tableM;
extern page_t header;

#define FIND_THREAD (1)
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
    sleep(5);
    ret = db_find(1,tmp_count+1, val, trid);
    if(ret == 0){
        printf("fisrt thread id : %d, trx id : %d, result : %s\n",id, trid, val);
    }
    sleep(10);
    

    ret = trx_commit(trid);
	return NULL;
}

int main(int argc, char ** argv){
    int ret = init_db(3,"logfile.data");
    int tableNum = open_table(argv[1]);    
    print_tree(tableNum);

    int id1 = 0; 
    int id2 = 0;
    int id3 = 0;
    int id4 = 0;
    // int ret = 0;
//for redo test log
    id1 = trx_begin();
    ret = db_update(tableNum,0,"a",id1);
    id2 = trx_begin();
    ret = db_update(tableNum,1,"a",id2);
    ret = db_update(tableNum,2,"a",id2);
    ret = db_update(tableNum,3,"a",id1);
    ret = db_update(tableNum,4,"a",id2);
    id1 = trx_commit(id1);
    id3 = trx_begin();
    id2 = trx_abort(id2);
    id4 = trx_begin();
    ret = db_update(tableNum,5,"a",id3);
    ret = db_update(tableNum,6,"a",id4);

//for undo test log
    // id1 = trx_begin();
    // ret = db_update(tableNum,0,"a",id1);
    // id2 = trx_begin();
    // ret = db_update(tableNum,1,"a",id2);
    // ret = db_update(tableNum,2,"a",id2);
    // ret = db_update(tableNum,3,"a",id1);
    // ret = db_update(tableNum,4,"a",id2);
    // id1 = trx_commit(id1);
    // id3 = trx_begin();
    // id2 = trx_abort(id2);
    // id4 = trx_begin();
    // ret = db_update(tableNum,5,"a",id3);
    // ret = db_update(tableNum,6,"a",id4);

//for undo test log 2
   //  id1 = trx_begin();
   //  ret = db_update(tableNum,0,"a",id1);
   //  id2 = trx_begin();
   //  ret = db_update(tableNum,1,"a",id2);
   //  ret = db_update(tableNum,2,"a",id2);
   //  ret = db_update(tableNum,3,"a",id1);
   //  ret = db_update(tableNum,4,"a",id2);
   //  // id1 = trx_commit(id1);
   //  id3 = trx_begin();
   // // id2 = trx_abort(id2);
   //  id4 = trx_begin();
   //  ret = db_update(tableNum,5,"a",id3);
   //  ret = db_update(tableNum,6,"a",id4);

   //  print_tree2(tableNum);

   //  trx * trx4 = trx_find(id4);
   //  log_buff_write(trx4->last_LSN, trx4->id,LOG_COM_TYPE, tableNum, 96, 0, "a", "6");

    log_buff_flush();
    
    print_tree(tableNum);
    
    log_buff_read_all_file();

    return 0;
    
}

