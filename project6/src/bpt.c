#include "bpt.h"

page_t header;
tManager * tableM;
buffer * buff;
log_buffer * log_buff;
int log_file;
FILE * log_msg_file;
recoveryManager * recovM;

pthread_mutex_t header_latch = PTHREAD_MUTEX_INITIALIZER;

#define HASH_SIZE 500

Node * hash_table[HASH_SIZE];
pthread_mutex_t lock_table_latch = PTHREAD_MUTEX_INITIALIZER;


//trxM
trxManager * trxM;
pthread_mutex_t trx_table_latch = PTHREAD_MUTEX_INITIALIZER;

//deadlock
graphManager * graphM;
visitManager * visitM;

int * Gqueue;
int GqueueLen = 0;

pthread_mutex_t graph_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gqueue_latch = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t visit_latch = PTHREAD_MUTEX_INITIALIZER;


pthread_mutex_t log_buff_latch = PTHREAD_MUTEX_INITIALIZER;

int open_table(char* pathname){
    int ret = open_table_sec(pathname);

    return ret;
}

int close_table(int table_id){
    int ret = close_table_sec(table_id);
    
    return ret;
}


int init_db(int num_buf, int flag, int log_num, char * log_path, char * logmsg_path){
    int ret = init_buff(num_buf);
    if(ret == -1){
        return -1;
    }
    ret = init_lock_table();
    init_trxM();
    init_graphM();
    init_visitM();
    ret = init_log_buff(num_buf, log_path, logmsg_path);
    init_recoveryM();
    printf("Init recoveryM succ\n");

    log_buff_read_all_file();

    if(flag == 0){
        recov_analysis();
        recov_redo(flag, log_num);
        recov_undo(flag, log_num);
    }
    if(flag == 1){
        recov_analysis();
        recov_redo(flag, log_num);
    }
    if(flag == 2){
        recov_analysis();
        recov_redo(flag, log_num);
        recov_undo(flag, log_num);
    }


    log_buff_flush();
    
    int tableNum = open_table("DATA1");  
    tableNum = open_table("DATA2");
    tableNum = open_table("DATA3");
    tableNum = open_table("DATA4");
    tableNum = open_table("DATA5");
    tableNum = open_table("DATA6");
    tableNum = open_table("DATA7");
    tableNum = open_table("DATA8");
    tableNum = open_table("DATA9");
    tableNum = open_table("DATA10");
    int ret2 = 0;
    ret = buff_clean();
    // printf("here?\n");
    // for(int i=1; i<11;i++){
    //     ret2 = close_table(i);
    // }

    // printf("End recovery tree\n");
    // print_tree(tableNum);
    // log_buff_read_all_file();
    // exit(1);

    ret = 0;
    return ret;
}


int shutdown_db(){
    int ret = buff_clean();
    if(ret == -1){
        return ret;
    }
    print_buff();
    free(buff->blocks);
    free(buff);
    return ret;
}

pagenum_t * queue;
uint64_t queueLen = 0;

void enqueue( pagenum_t pNum ) {
    
    if (queueLen == 0) {
        queue = (pagenum_t*)malloc(sizeof(pagenum_t));
        queueLen += 1;
        queue[queueLen-1] = pNum;
    }

    else {
        queueLen += 1;
        queue = (pagenum_t*)realloc(queue,sizeof(pagenum_t)*queueLen);
        queue[queueLen-1] = pNum;
    }
}


pagenum_t dequeue( void ) {
    pagenum_t ret_i = queue[0];
    for(int i=0; i < (queueLen-1); i++){
        queue[i] = queue[i+1];
    }
    queue[queueLen-1] = 0;
    queueLen -= 1;
    queue = (pagenum_t*)realloc(queue,sizeof(pagenum_t)*queueLen);

    return ret_i;
}


void print_tree(int table_id) { //print
 
    int i = 0;

    buff_read_page(table_id,0,&header);

    if (header.H.rootPnum == 0) {
        printf("Empty tree.\n");
        return;
    }
    queueLen = 0;

    enqueue(header.H.rootPnum);
    while( queueLen != 0 ){
        uint64_t tmp_len = queueLen;
        for(int i = 0; i<tmp_len; i++){
            pagenum_t tmp = dequeue();
            page_t tmp_page;
            buff_read_page(table_id, tmp, &tmp_page);
            uint32_t howMany = tmp_page.I.header.numKeys; //internal이나 leaf나 key의 갯수는 node에 있는 key의 갯수를 지칭하므로

            if( tmp_page.I.header.isLeaf == 0 ) {  //internal page인 경우, 자식 page를 queue에 넣기 & page의 key 출력
                enqueue(tmp_page.I.header.special);
                for(int j=0; j<howMany; j++){
                    enqueue(tmp_page.I.records[j].pageN);
                }
                for(int j=0; j<howMany; j++){
                printf("%lld ",tmp_page.I.records[j].key);
                }
            }

            else{  //leaf page인 경우 key만 출력
                for(int j=0; j<howMany; j++){ 
                printf("%lld[%s] ",tmp_page.L.records[j].key, tmp_page.L.records[j].value);
                }
            }
        printf("| ");
        } 
        printf("\n");
    }
    free(queue);
}

void print_tree2(int table_id) {
    buff_read_page(table_id, 0, &header);

    int i = 0;

    if (header.H.rootPnum == 0) {
        printf("Empty tree.\n");
        return;
    }
    queueLen = 0;

    enqueue(header.H.rootPnum);
    while( queueLen != 0 ){
        uint64_t tmp_len = queueLen;
        for(int i = 0; i<tmp_len; i++){
            pagenum_t tmp = dequeue();
            // printf("for debug node pagenum_t : %llu   ",tmp);
            page_t tmp_page;
            buff_read_page(table_id, tmp, &tmp_page);
            uint32_t howMany = tmp_page.I.header.numKeys; //internal이나 leaf나 key의 갯수는 node에 있는 key의 갯수를 지칭하므로
            printf("for debug node page_LSN : %llu   ",tmp_page.I.header.page_LSN);

            if( tmp_page.I.header.isLeaf == 0 ) {  //internal page인 경우, 자식 page를 queue에 넣기 & page의 key 출력
                enqueue(tmp_page.I.header.special);
                for(int j=0; j<howMany; j++){
                    enqueue(tmp_page.I.records[j].pageN);
                }
                for(int j=0; j<howMany; j++){
                printf("%lld ",tmp_page.I.records[j].key);
                }
            }

            else{  //leaf page인 경우 key만 출력
                for(int j=0; j<howMany; j++){ 
                printf("%lld[%s] ",tmp_page.L.records[j].key, tmp_page.L.records[j].value);
                }
            }
        printf("| ");
        } 
        printf("\n");
    }
    free(queue);
}

pagenum_t find_leaf( int table_id, int64_t key ) {
    // printf("A\n");
    pthread_mutex_lock(&header_latch);
    buff_read_page(table_id, 0, &header);
    // buff_release_page_latch(table_id, 0);
    // printf("B\n");

    int i = 0;
    pagenum_t tmp_pageN = header.H.rootPnum;
    pthread_t tmp_id = pthread_self();
    page_t tmp_page;
    pthread_mutex_unlock(&header_latch);
    
    if(tmp_pageN == 0){
        return 0;
    }
    // pthread_mutex_unlock(&header_latch);
    
    buff_read_page(table_id, tmp_pageN, &tmp_page);
    // buff_release_page_latch(table_id, tmp_pageN);

    while(tmp_page.I.header.isLeaf == 0){

        i = 0;
        while(i < tmp_page.I.header.numKeys ){
            if( key >= tmp_page.I.records[i].key   ) i++;
            else break;
        }
        i -= 1;
        if(i == -1 ){
            tmp_pageN = tmp_page.I.header.special;
        }
        else {
            tmp_pageN = tmp_page.I.records[i].pageN;
        }
        
        buff_read_page(table_id, tmp_pageN, &tmp_page);
        // buff_release_page_latch(table_id, tmp_pageN);
    }

    return tmp_pageN;
}

int db_find(int table_id, int64_t key, char * ret_val, int trx_id ) {
    int tmp_id = pthread_self();

    if (tableM->tables[table_id-1].dis < 0){
        int ret = trx_abort(trx_id);
        printf("open_table fail\n");
        return 1;
    }
    int i = 0;
    pagenum_t leafN = find_leaf( table_id, key );
    if(leafN == 0){
        int ret = trx_abort(trx_id);
        return 1;
    }
    page_t leaf;
    
    buff_read_page_new(table_id, leafN, &leaf);
    
    for( i = 0; i < leaf.L.header.numKeys; i++){
        if(leaf.L.records[i].key == key){
            // lock_t * ret_obj = NULL;
            lock_t * ret_obj;

            int ret_int = lock_acquire(table_id, key, trx_id, 0, leaf.L.records[i].value, &ret_obj);
            // printf("IN db_find, ret_int %d \n",ret_int);
            // printf("IN db_find, lock_obj owner_id %d \n",ret_obj->owner_id);
            if(ret_int == -1){
                return 2;
            }
            if(ret_int == 0 ){ //안기다리고 lock을 획득 했을때
                
                memcpy(ret_val, leaf.L.records[i].value, VALUE_SIZE);
                buff_release_page_latch(table_id, leafN);
                return 0;
            }
            if(ret_int == 1){ //기다려야 할때
                // buff_release_page_latch(table_id, leafN);
                lock_wait(ret_obj, table_id, leafN);
                buff_read_page_new(table_id, leafN, &leaf);
                memcpy(ret_val, leaf.L.records[i].value, VALUE_SIZE);
                buff_release_page_latch(table_id, leafN);
                return 0;
            }
            if(ret_int == 2){ //dead lock
                buff_release_page_latch(table_id, leafN);
                int ret_abort = trx_abort(trx_id);
                if(ret_abort == 0){
                    printf("trx_abort fail\n");
                    exit(1);
                }
                return 1;
            }
        }
    }
    buff_release_page_latch(table_id, leafN);
    int ret = trx_abort(trx_id); //insert 할때 문제생김
    return 1;
}

int db_update(int table_id, int64_t key, char * values, int trx_id ) {
    // printf("tableid : %d\n", tableM->tables[table_id-1].dis);
    if (tableM->tables[table_id-1].dis < 0){
        int ret = trx_abort(trx_id);
        printf("open_table fail\n");
        return 1;
    }
    int i = 0;

    // printf("find leaf ok?\n");
    pagenum_t leafN = find_leaf( table_id, key );
    // printf("find leaf ok?\n");
    
    if(leafN == 0){
        int ret = trx_abort(trx_id);
        return 1;
    }
    page_t leaf;
    int tmp_idx;

    buff_read_page_new(table_id, leafN, &leaf);
    
    
    for( i = 0; i < leaf.L.header.numKeys; i++){
        if(leaf.L.records[i].key == key){
            // lock_t * ret_obj = NULL;
            lock_t * ret_obj;
            int ret_int = lock_acquire(table_id, key, trx_id, 1, leaf.L.records[i].value, &ret_obj);
            if(ret_int == -1){
                // printf("IN db_update, lock acq fail\n");
                return 2;
            }

            if(ret_int == 0){

                //write update log, log의 old_val에 현재 page의 값, new_val에 인자로 들어온 값 => redo 할때 일괄 처리 할라고
                // pthread_mutex_lock(&lock_table_latch);
                pthread_mutex_lock(&trx_table_latch);

                pthread_mutex_lock(&log_buff_latch);
                int64_t tmp_LSN = log_buff->global_LSN; //현재 쓰여질 log의 LSN임, log_buff_write하고 나면 global_LSN 바뀜.
                trx * tar_trx = trx_find(trx_id);
                log_buff_write(tar_trx->last_LSN, trx_id, LOG_UPD_TYPE, table_id, leafN, i, leaf.L.records[i].value, values);
                tar_trx->last_LSN = tmp_LSN;
                pthread_mutex_unlock(&log_buff_latch);

                // pthread_mutex_unlock(&lock_table_latch);
                pthread_mutex_unlock(&trx_table_latch);


                buff_update_read_page(table_id, leafN, &leaf);
                leaf.L.header.page_LSN = tmp_LSN;
                memcpy(leaf.L.records[i].value, values ,VALUE_SIZE);
                buff_update_write_page(table_id, leafN, &leaf);
                return 0;
            }

            if(ret_int == 1){
                lock_wait(ret_obj, table_id, leafN);
                buff_get_page_latch(table_id, leafN);

                //write update log, log의 old_val에 현재 page의 값, new_val에 인자로 들어온 값 => redo 할때 일괄 처리 할라고
                // pthread_mutex_lock(&lock_table_latch);
                pthread_mutex_lock(&trx_table_latch);

                pthread_mutex_lock(&log_buff_latch);
                int64_t tmp_LSN = log_buff->global_LSN; //현재 쓰여질 log의 LSN임, log_buff_write하고 나면 global_LSN 바뀜.
                trx * tar_trx = trx_find(trx_id);
                log_buff_write(tar_trx->last_LSN, trx_id, LOG_UPD_TYPE, table_id, leafN, i, leaf.L.records[i].value, values);
                tar_trx->last_LSN = tmp_LSN;
                pthread_mutex_unlock(&log_buff_latch);

                // pthread_mutex_unlock(&lock_table_latch);
                pthread_mutex_unlock(&trx_table_latch);


                buff_update_read_page(table_id, leafN, &leaf);
                leaf.L.header.page_LSN = tmp_LSN;
                memcpy(leaf.L.records[i].value, values ,VALUE_SIZE);
                buff_update_write_page(table_id, leafN, &leaf);
                return 0;
            }
            
            if(ret_int == 2){
                buff_release_page_latch(table_id, leafN);
                // printf("IN db_update, deadlock detect\n");
                int ret_abort = trx_abort(trx_id);
                if(ret_abort == 0){
                    printf("trx_abort fail\n");
                    exit(1);
                }
                return 1;
            }         
        }
    }

    buff_release_page_latch(table_id, leafN);
    int ret = trx_abort(trx_id);
    return 1;
}


int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


// INSERTION
pagenum_t make_internal( int table_id ) {
    pagenum_t tmp_pageN = buff_alloc_page(table_id);
    page_t tmp_page;
    buff_read_page(table_id, tmp_pageN,&tmp_page);

    tmp_page.I.header.parent = 0;
    tmp_page.I.header.isLeaf = 0;
    tmp_page.I.header.numKeys = 0;
    tmp_page.I.header.special = 0;
    buff_write_page(table_id, tmp_pageN, &tmp_page);

    return tmp_pageN;
}

pagenum_t make_leaf( int table_id ) {
    pagenum_t tmp_pageN = buff_alloc_page(table_id);
    page_t tmp_page;
    buff_read_page(table_id, tmp_pageN,&tmp_page);

    tmp_page.L.header.parent = 0;
    tmp_page.L.header.isLeaf = 1;
    tmp_page.L.header.numKeys = 0;
    tmp_page.L.header.page_LSN = -1;
    tmp_page.L.header.special = 0;
    buff_write_page(table_id, tmp_pageN, &tmp_page);

    // printf("END make_leaf \n");

    return tmp_pageN;
}

void make_tmpinternal( tmppage_t * tmp_page ) {

    tmp_page->I.header.parent = 0;
    tmp_page->I.header.isLeaf = 0;
    tmp_page->I.header.numKeys = 0;
    tmp_page->I.header.special = 0;

}

void make_tmpleaf( tmppage_t * tmp_page ) {

    tmp_page->L.header.parent = 0;
    tmp_page->L.header.isLeaf = 1;
    tmp_page->L.header.numKeys = 0;
    tmp_page->L.header.page_LSN = -1;
    tmp_page->L.header.special = 0;
}


int get_left_index(int table_id, pagenum_t parentN, pagenum_t leftN) {

    int left_index = 0;
    page_t parent, left;
    buff_read_page(table_id, parentN, &parent);
    buff_read_page(table_id, leftN, &left);
    if (parent.I.header.special == leftN) return -1;
    while (left_index <= parent.I.header.numKeys-1 && 
            parent.I.records[left_index].pageN != leftN)
        left_index++;
    
    return left_index;
}

pagenum_t insert_into_leaf( int table_id, pagenum_t leaf_pageN, int64_t key, char * value ) {

    int i, insertion_point;
    page_t leaf_page;
    buff_read_page(table_id, leaf_pageN,&leaf_page);

    insertion_point = 0;
    while( insertion_point < leaf_page.L.header.numKeys && leaf_page.L.records[insertion_point].key < key )
        insertion_point++;

    for(i = leaf_page.L.header.numKeys; i > insertion_point; i--){
        leaf_page.L.records[i].key = leaf_page.L.records[i-1].key;
        memcpy(leaf_page.L.records[i].value,leaf_page.L.records[i-1].value,VALUE_SIZE);
    }
    
    leaf_page.L.records[insertion_point].key = key;
    memcpy(leaf_page.L.records[insertion_point].value, value, VALUE_SIZE);
    leaf_page.L.header.numKeys++;
    
    
    buff_write_page(table_id, leaf_pageN,&leaf_page);

    return leaf_pageN;
}

pagenum_t insert_into_leaf_after_splitting( int table_id, pagenum_t leaf_pageN, int64_t key, char * value) {
    page_t leaf;
    buff_read_page(table_id, leaf_pageN, &leaf);

    pagenum_t new_leafN = make_leaf(table_id);
    page_t new_leaf;
    buff_read_page(table_id, new_leafN, &new_leaf);

    tmppage_t * tmp_leaf = (tmppage_t *)malloc(sizeof(tmppage_t));
    make_tmpleaf(tmp_leaf);
    
    int insertion_index = 0;
    while(insertion_index < LEAF_ORDER-1 && leaf.L.records[insertion_index].key < key)
        insertion_index++;
    
    int i, j;
    for ( i = 0, j=0; i < leaf.L.header.numKeys; i++, j++  ){
        if ( j == insertion_index ) j++;
        tmp_leaf->L.records[j].key = leaf.L.records[i].key;
        memcpy(tmp_leaf->L.records[j].value, leaf.L.records[i].value,VALUE_SIZE);
    }

    
    tmp_leaf->L.records[insertion_index].key = key;
    memcpy(tmp_leaf->L.records[insertion_index].value, value,VALUE_SIZE);

    leaf.L.header.numKeys = 0;

    int split = cut( LEAF_ORDER-1 );

    for ( i = 0; i < split; i++) {
        leaf.L.records[i].key = tmp_leaf->L.records[i].key;
        memcpy(leaf.L.records[i].value, tmp_leaf->L.records[i].value,VALUE_SIZE);
        leaf.L.header.numKeys += 1;
    }

    for( i = split, j = 0; i < LEAF_ORDER; i++, j++) {
        new_leaf.L.records[j].key = tmp_leaf->L.records[i].key;
        memcpy(new_leaf.L.records[j].value, tmp_leaf->L.records[i].value,VALUE_SIZE);
        new_leaf.L.header.numKeys += 1;
    }
    

    // file_free_page(tmp_leafN);
    free(tmp_leaf);

    new_leaf.L.header.special = leaf.L.header.special;
    leaf.L.header.special = new_leafN;

    //안쓰는 공간 0으로 초기화
    for( i = leaf.L.header.numKeys; i < LEAF_ORDER-1 ; i++)
        memset(leaf.L.records[i].value, 0, VALUE_SIZE);
    for( i = new_leaf.L.header.numKeys; i < LEAF_ORDER-1 ; i++)
        memset(new_leaf.L.records[i].value, 0, VALUE_SIZE);

    new_leaf.L.header.parent = leaf.L.header.parent;
    int64_t new_key = new_leaf.L.records[0].key;

    buff_write_page(table_id, leaf_pageN,&leaf);
    buff_write_page(table_id, new_leafN,&new_leaf);

    return insert_into_parent(table_id, leaf_pageN, new_key, new_leafN);
}

pagenum_t insert_into_node( int table_id, pagenum_t nN, 
        int left_index, int64_t key, pagenum_t rightN) {
    int i;
    page_t n;
    buff_read_page(table_id,nN, &n);    

    for (i = n.I.header.numKeys-1; i > left_index; i--){
        n.I.records[i+1].pageN = n.I.records[i].pageN; //pageNum 밀어주기
        n.I.records[i+1].key = n.I.records[i].key; //key 밀기
    }

    n.I.records[left_index+1].pageN = rightN;
    n.I.records[left_index+1].key = key;
    n.I.header.numKeys += 1;
    buff_write_page(table_id, nN, &n);
    
    return 0;
}

pagenum_t insert_into_node_after_splitting( int table_id, pagenum_t old_pageN, int left_index, 
        int64_t key, pagenum_t rightN) {
    page_t old_page;
    buff_read_page(table_id, old_pageN, &old_page);

    pagenum_t new_pageN = make_internal(table_id);
    page_t new_page;
    buff_read_page(table_id, new_pageN, &new_page);

    
    tmppage_t * tmp_page = (tmppage_t*) malloc(sizeof(tmppage_t));
    make_tmpinternal(tmp_page);
    

    int i, j, split;
    int64_t k_prime;

    left_index++;
    tmp_page->I.header.special = old_page.I.header.special;
    for( i = 0, j = 0; i < old_page.I.header.numKeys; i++, j++) {
        if( j == left_index ) j++;
        tmp_page->I.records[j].key = old_page.I.records[i].key;
        tmp_page->I.records[j].pageN = old_page.I.records[i].pageN;
    }
    tmp_page->I.records[left_index].key = key;
    tmp_page->I.records[left_index].pageN = rightN;

    split = cut(INTERNAL_ORDER);

    old_page.I.header.numKeys = 0;
    old_page.I.header.special = tmp_page->I.header.special;
    for( i =0; i < split -1; i++ ){
        old_page.I.records[i].key = tmp_page->I.records[i].key;
        old_page.I.records[i].pageN = tmp_page->I.records[i].pageN;
        old_page.I.header.numKeys += 1;
    }
    old_page.I.records[split-1].key = 0; //마지막거 없어졌으니 초기화해주기
    old_page.I.records[split-1].pageN = 0;

    k_prime = tmp_page->I.records[split-1].key;

    new_page.I.header.special = tmp_page->I.records[split-1].pageN;
    for(++i, j=0; i < INTERNAL_ORDER; i++, j++){
        new_page.I.records[j].key = tmp_page->I.records[i].key;
        new_page.I.records[j].pageN = tmp_page->I.records[i].pageN;
        new_page.I.header.numKeys += 1;
    }
    new_page.I.header.parent = old_page.I.header.parent;

    free(tmp_page);

    pagenum_t childN = new_page.I.header.special;
    page_t child;
    buff_read_page(table_id, childN,&child);
    child.I.header.parent = new_pageN;
    buff_write_page(table_id, childN,&child);

    for ( i = 0; i< new_page.I.header.numKeys; i++){
        childN = new_page.I.records[i].pageN;
        buff_read_page(table_id,childN,&child);
        child.I.header.parent = new_pageN;
        buff_write_page(table_id,childN,&child);
    }

    buff_write_page(table_id,old_pageN,&old_page);
    buff_write_page(table_id,new_pageN,&new_page);

    return insert_into_parent( table_id, old_pageN, k_prime, new_pageN);
}

pagenum_t insert_into_parent( int table_id, pagenum_t leftN, int64_t key, pagenum_t rightN) {

    int left_index;

    page_t leftP, rightP, parentP;
    buff_read_page(table_id, leftN, &leftP);
    buff_read_page(table_id, rightN, &rightP);

    pagenum_t parentN = leftP.I.header.parent;
    buff_read_page(table_id, parentN, &parentP);

    if ( parentN == 0 ){
        return insert_into_new_root(table_id, leftN, key, rightN);
    }


    left_index = get_left_index(table_id, parentN, leftN);

    if ( parentP.I.header.numKeys < INTERNAL_ORDER-1 ){
        return insert_into_node(table_id, parentN, left_index, key, rightN);
    }

    return insert_into_node_after_splitting( table_id, parentN, left_index, key, rightN );
}

pagenum_t insert_into_new_root(int table_id, pagenum_t leftN, int64_t key, pagenum_t rightN) {

    pagenum_t rootN = make_internal(table_id);
    page_t root, left, right;
    buff_read_page(table_id, rootN, &root);
    root.I.records[0].key = key;
    root.I.header.special = leftN;
    root.I.records[0].pageN = rightN;
    root.I.header.numKeys += 1;
    root.I.header.parent = 0;
    
    buff_read_page(table_id, leftN, &left);
    buff_read_page(table_id, rightN, &right);
    left.I.header.parent = rootN;
    right.I.header.parent = rootN;

    buff_write_page(table_id, rootN, &root);
    buff_write_page(table_id, leftN, &left);
    buff_write_page(table_id, rightN, &right);

    buff_read_page(table_id, 0, &header);
    header.H.rootPnum = rootN;
    buff_write_page(table_id, 0, &header);

    return rootN;
}

void start_new_tree(int table_id, int64_t key, char * value) {

    pagenum_t rootN = make_leaf(table_id);
    page_t root;
    buff_read_page(table_id, rootN,&root);
    
    root.L.header.parent = 0;
    root.L.header.numKeys += 1;
    root.L.records[0].key = key;
    memcpy(root.L.records[0].value, value, VALUE_SIZE);
    buff_write_page(table_id, rootN,&root);

    buff_read_page(table_id, 0, &header);
    header.H.rootPnum = rootN;
    buff_write_page(table_id, 0,&header);
    
    buff_read_page(table_id, rootN,&root);
}


int db_insert(int table_id, int64_t key, char* value){
    if (tableM->tables[table_id-1].dis < 0){
        printf("open_table fail\n");
        return 1;
    }
    char * ret_val = (char*)malloc(sizeof(char)*120);
    pagenum_t rootN;
    if(db_find(table_id, key,ret_val,1) == 0 ){ //trx id 1로 임의로 준거임
        return 1;
    }
    // print_trxM();

    buff_read_page(table_id,0,&header);
    if(header.H.rootPnum == 0){
        start_new_tree(table_id, key, value);
        return 0;
    }

    pagenum_t leaf_pageN = find_leaf(table_id, key);
    page_t leaf_page;
    buff_read_page(table_id, leaf_pageN,&leaf_page);

    if(leaf_page.L.header.numKeys < LEAF_ORDER-1){
        rootN = insert_into_leaf(table_id, leaf_pageN, key, value);
        return 0;
    }

    rootN = insert_into_leaf_after_splitting(table_id, leaf_pageN, key, value);
    return 0;

}


// DELETION
int get_neighbor_index( int table_id, pagenum_t nN ) {
    int i;
    pagenum_t parentN;
    page_t n, parent;
    buff_read_page(table_id,nN,&n);
    parentN = n.I.header.parent;
    buff_read_page(table_id, parentN, &parent);

    if(parent.I.header.special == nN) return -1;
    for( i = 0; i <parent.I.header.numKeys;i++){ //ex) 0번째 포인터가 나를 가리키고 있으면 나는 1번째이고 index 0이 return 되는게 옳다
        if(parent.I.records[i].pageN == nN)
            return i;
    }
    printf("Search for nonexistent pointer to node in parent.\n");
    exit(EXIT_FAILURE);

}


pagenum_t remove_entry_from_node( int table_id, pagenum_t nN, int64_t key ) {

    int i = 0;
    page_t n;
    buff_read_page(table_id,nN,&n);
    int flag = n.I.header.isLeaf ? 1 : 0;

    if(flag == 0){ //Internal_page
        while ( n.I.records[i].key != key )
            i++;
        for (int j = i+1; j < n.I.header.numKeys; j++){
            n.I.records[j-1].key = n.I.records[j].key;
        }
        
        for (int j = i+1; j < n.I.header.numKeys; j++){
            n.I.records[j-1].pageN = n.I.records[j].pageN;
        }
        n.I.header.numKeys -= 1;
        
        for( int j = n.I.header.numKeys; j < INTERNAL_ORDER-1; j++  ){
            n.I.records[j].key = 0;
            n.I.records[j].pageN = 0;
        }
    }

    else{ //leaf_page
        while ( n.L.records[i].key != key )
            i++;
        for (++i; i < n.L.header.numKeys; i++){
            n.L.records[i-1].key = n.L.records[i].key;
            memcpy(n.L.records[i-1].value,n.L.records[i].value,VALUE_SIZE);
        }
        n.I.header.numKeys -= 1;

        for( int j = n.L.header.numKeys; j < LEAF_ORDER-1; j++  ){
            n.L.records[j].key = 0;
            memset(n.L.records[j].value,0,VALUE_SIZE);
        }
    }

    buff_write_page(table_id,nN,&n);
    return nN;
}


pagenum_t adjust_root(int table_id, pagenum_t rootN) {

    page_t root;
    buff_read_page(table_id, rootN, &root);
    pagenum_t new_rootN;
    page_t new_root;
    int flag = root.I.header.isLeaf ? 1 : 0;
    buff_read_page(table_id, 0, &header);

    if(root.I.header.numKeys > 0 ){
        return rootN;
    }

    if(flag == 0){
        header.H.rootPnum = root.I.header.special;
        
        new_rootN = root.I.header.special;
        buff_read_page(table_id, new_rootN, &new_root);
        new_root.I.header.parent = 0;

        buff_write_page(table_id, 0,&header);
        buff_write_page(table_id,new_rootN,&new_root);
    }
    else{
        header.H.rootPnum = 0;
        new_rootN = 0;

        buff_write_page(table_id,0,&header);
    }
    buff_free_page(table_id,rootN);

    return new_rootN;

}

pagenum_t coalesce_nodes( int table_id, pagenum_t nN, pagenum_t neighborN, int neighbor_index, int64_t k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    pagenum_t tmpN, rootN;

    if (neighbor_index == -1) {
        tmpN = nN;
        nN = neighborN;
        neighborN = tmpN;
    } //nN이 1 neighborN이 0

    page_t n, neighbor;
    buff_read_page(table_id, nN, &n);
    buff_read_page(table_id, neighborN, &neighbor);

    pagenum_t parentN = n.I.header.parent;

    neighbor_insertion_index = neighbor.I.header.numKeys; //internal이나 leaf나 header는 똑같음

    if (n.I.header.isLeaf == 0) { //internal 일때
        neighbor.I.records[neighbor_insertion_index].key = k_prime; //부모에서 내려온 값
        neighbor.I.records[neighbor_insertion_index].pageN = n.I.header.special; //neighbor의 제일 왼쪽 자식
        neighbor.I.header.numKeys += 1;
        n_end = n.I.header.numKeys;
        for( i = neighbor_insertion_index + 1, j=0; j < n_end; i++, j++) {
            neighbor.I.records[i].key = n.I.records[j].key;
            neighbor.I.records[i].pageN = n.I.records[j].pageN;
            neighbor.I.header.numKeys += 1;
            n.I.header.numKeys -= 1;
        } //여기까지 해서 n의 모든걸 neighbor로 옮김
        
        pagenum_t childN = n.I.header.special;
        page_t child;
        buff_read_page(table_id,childN, &child);
        child.I.header.parent = neighborN;
        buff_write_page(table_id,childN,&child);
        for( i = 0; i < neighbor.I.header.numKeys; i++){
            childN = neighbor.I.records[i].pageN;
            buff_read_page(table_id,childN, &child);
            child.I.header.parent = neighborN;
            buff_write_page(table_id,childN, &child);
        }
    }

    else {
        for ( i = neighbor_insertion_index, j=0; j < n.L.header.numKeys; i++, j++ ){
            neighbor.L.records[i].key = n.L.records[j].key;
            memcpy(neighbor.L.records[i].value, n.L.records[j].value,VALUE_SIZE);
            neighbor.L.header.numKeys += 1;
        }
        neighbor.L.header.special = n.L.header.special;
    }
    

    buff_write_page(table_id,neighborN,&neighbor);
    buff_write_page(table_id,nN,&n);
    rootN = delete_entry(table_id,parentN, k_prime);
    buff_free_page(table_id,nN);
    
    return rootN;
}


pagenum_t redistribute_nodes( int table_id, pagenum_t nN, pagenum_t neighborN, int neighbor_index, 
        int k_prime_index, int64_t k_prime) {  

    int i;
    pagenum_t tmpN, parentN;
    page_t n, neighbor, tmp, parent;
    buff_read_page(table_id, nN, &n);
    buff_read_page(table_id, neighborN, &neighbor);
    

    if (neighbor_index != -1) {
        n.I.records[0].pageN = n.I.header.special;  //n의 special 한칸 밀어주고
        n.I.header.special = neighbor.I.records[neighbor.I.header.numKeys-1].pageN; //special에 neighbor의 마지막 pageN 읽어오고
        tmpN = neighbor.I.records[neighbor.I.header.numKeys-1].pageN; //tmp에 neighbor의 마지막 자식 불러와서 parent를 n으로 설정
        buff_read_page(table_id,tmpN,&tmp);
        tmp.I.header.parent = nN;
        neighbor.I.records[neighbor.I.header.numKeys-1].pageN = 0; //neighbor의 마지막 parent 초기화
        n.I.records[0].key = k_prime; //n에 새로운 키값 넣어주고
        parentN = n.I.header.parent; //parnet page 불러와서 parnet 페이지 조정해주기
        buff_read_page(table_id,parentN, &parent);
        parent.I.records[k_prime_index].key = neighbor.I.records[neighbor.I.header.numKeys-1].key;

    }

    else {  
        n.I.records[n.I.header.numKeys].key = k_prime;  //neighbor에서 가져온 key값과 자식 설정
        n.I.records[n.I.header.numKeys].pageN = neighbor.I.header.special;
        tmpN = neighbor.I.header.special; //tmp에 neighbor의 첫번째 자식 불러와서 parent를 n으로 설정
        buff_read_page(table_id,tmpN,&tmp);
        tmp.I.header.parent = nN;
        parentN = n.I.header.parent; //parnet page 불러와서 parnet 페이지 조정해주기
        buff_read_page(table_id,parentN, &parent);
        parent.I.records[k_prime_index].key = neighbor.I.records[0].key;

        neighbor.I.header.special = neighbor.I.records[0].pageN;  //neighbor의 key와 자식들 한칸씩 당겨주기
        for( i = 0; i<neighbor.I.header.numKeys-1 ; i++){
            neighbor.I.records[i].key = neighbor.I.records[i+1].key;
            neighbor.I.records[i].pageN = neighbor.I.records[i+1].pageN;
        }
        neighbor.I.records[i].key = 0; //마지막에 들어있던 값 0으로 초기화
        neighbor.I.records[i].pageN = 0;

        
    }

    n.I.header.numKeys += 1;
    neighbor.I.header.numKeys -=1;

    buff_write_page(table_id,nN,&n);
    buff_write_page(table_id,tmpN,&tmp);
    buff_write_page(table_id,parentN,&parent);
    buff_write_page(table_id,neighborN,&neighbor);

    return nN;
}

pagenum_t delete_entry( int table_id, pagenum_t nN, int64_t key ) {
    int min_keys;
    int neighbor_index;
    int k_prime_index;
    int64_t k_prime;
    page_t n;
    buff_read_page(table_id, nN,&n);
    pagenum_t parentN = n.I.header.parent;
    page_t parent;
    buff_read_page(table_id,parentN,&parent);
    pagenum_t neighborN;

    nN = remove_entry_from_node(table_id,nN,key);
    buff_read_page(table_id,nN,&n); //remove_entry_from_node의 변경된 데이터를 다시 읽어와야함

    buff_read_page(table_id, 0, &header);
    if (nN == header.H.rootPnum) 
        return adjust_root(table_id, header.H.rootPnum);

    if(n.I.header.numKeys != 0){
        return nN;
    }

    neighbor_index = get_neighbor_index( table_id, nN );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = parent.I.records[k_prime_index].key; //parent는 무조건 internal node니까
    if(neighbor_index == -1 ){
        neighborN = parent.I.records[0].pageN;
    }
    else if (neighbor_index == 0){
        neighborN = parent.I.header.special;
    }
    else{
        neighborN = parent.I.records[neighbor_index-1].pageN;
    }

    /* Coalescence. */
    page_t neighbor;
    buff_read_page(table_id, neighborN, &neighbor);
    int capacity = n.I.header.isLeaf ? LEAF_ORDER : INTERNAL_ORDER;

    if ( neighbor.I.header.numKeys+1 == capacity && n.I.header.isLeaf == 0 ){
        return redistribute_nodes(table_id, nN, neighborN, neighbor_index, k_prime_index, k_prime);
    }
    else{
        return coalesce_nodes(table_id, nN, neighborN, neighbor_index, k_prime);
    }

}



/* Master deletion function.
 */
int db_delete( int table_id, int64_t key ) {
    if (tableM->tables[table_id-1].dis < 0){
        printf("open_table fail\n");
        return 1;
    }
    pagenum_t rootN;
    char * ret_val = (char*)malloc(sizeof(char)*120);
    pagenum_t key_leaf = find_leaf(table_id, key);
    
    if ( db_find(table_id, key,ret_val, 1) == 0 && key_leaf != 0) { //trx id 임의로 준거임
        rootN = delete_entry(table_id, key_leaf, key);
        free(ret_val);
        return 0;
    }
    free(ret_val);
    return 1;
}




///////about hash
void print_hash_table()
{
	pthread_mutex_lock(&trx_table_latch);
	pthread_mutex_lock(&lock_table_latch);

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
				printf("%d, %lld. mode : ", node->lock_entry->tid, node->lock_entry->rid);
				while(tmp){
					count += 1;
					printf("%d ", tmp->mode);
					tmp = tmp->next;
				}
				// printf("%d, %lld // ", node->lock_entry->tid, node->lock_entry->rid);
				printf("   count : %d // \n",count);
                node = node->next;
            }
			lock_t * tmp = node->lock_entry->head;
			int count = 0;
			printf("%d, %lld. mode : ", node->lock_entry->tid, node->lock_entry->rid);
			while(tmp){
				count += 1;
				printf("%d ", tmp->mode);
				tmp = tmp->next;
			}
			// printf("%d, %lld // \n", node->lock_entry->tid, node->lock_entry->rid);
            // printf("%d, %lld, count : %d\n", node->lock_entry->tid, node->lock_entry->rid, count);
			printf("   count : %d // \n",count);
        }
    }
    printf("\n\n");

	pthread_mutex_unlock(&lock_table_latch);
	pthread_mutex_unlock(&trx_table_latch);
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
lock_t * add_obj(lock_e * entry, int trx_id, int lock_mode, char * value){
	lock_t * tmp = (lock_t *)malloc(sizeof(lock_t));
	tmp->prev = NULL;
	tmp->next = NULL;
	tmp->sentinel = entry;
	
	int ret = pthread_cond_init(&tmp->cv,0);

	tmp->mode = lock_mode;
	tmp->trxnextlock = NULL; 
	tmp->trxprevlock = NULL;
	tmp->owner_id = trx_id;
    
    memcpy(tmp->old_v, value, VALUE_SIZE);
	

	if(entry->head == NULL){
		entry->head = tmp;
		entry->tail = tmp;

		//add trx table
		ret = add_obj_trx(trx_id, tmp);
		if(ret == 1){
			return tmp;
		}
		return NULL;
	}

	lock_t * ori_tail = entry->head;
	while(ori_tail->next){
		ori_tail = ori_tail->next;
	}
	ori_tail->next = tmp;
	tmp->prev = ori_tail;
	entry->tail = tmp;
	
	//add trx table
	ret = add_obj_trx(trx_id, tmp);
	if(ret == 1){
		return tmp;
	}

	return NULL;
}

lock_t * del_obj(lock_t * obj){
	lock_e * entry = obj->sentinel;
	lock_t * tmp = obj->next;

	if(obj == entry->tail && obj == entry->head){
		entry->tail = NULL;
		entry->head = NULL;
	}

	else if(obj == entry->tail){
		entry->tail = obj->prev;
		if(obj->prev){
			obj->prev->next = NULL;
		}
	}
	else if (obj == entry->head){
		entry->head = obj->next;
		if(obj->next){
			obj->next->prev = NULL;
		}
	}
	else{
		obj->prev->next = obj->next;
		obj->next->prev = obj->prev;
	}


	obj->prev = NULL;
	obj->next = NULL;

	//trx table에서 삭제
	if(obj->trxprevlock != NULL){
		obj->trxprevlock->trxnextlock = NULL;
	}
	
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

int
lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, char * value, lock_t ** ret_lock)
{
    pthread_mutex_lock(&lock_table_latch);
	pthread_mutex_lock(&trx_table_latch);
	// pthread_mutex_lock(&lock_table_latch);
	lock_t * obj = NULL;
	Node * node = NULL;

	node = hash_find(table_id, key);
	
	if(node == NULL){
		node = hash_insert(table_id, key);
	}
	

	obj = add_obj(node->lock_entry, trx_id, lock_mode, value);
    

	if(obj == NULL){
		pthread_mutex_unlock(&lock_table_latch);
		pthread_mutex_unlock(&trx_table_latch);
		return -1;
	}
    // printf("IN lock_acq, lock_mode : %d\n",lock_mode);

	lock_t * prev_obj = obj->prev;
	if(lock_mode == 0){  //s lock
        if(obj->sentinel->head != NULL && obj->sentinel->head->owner_id == obj->owner_id && obj->sentinel->head->mode == 1){ //이미 x lock을 가지고 있는 상황
            lock_t * ret = obj->sentinel->head;
            lock_t * tmp = del_obj(obj);
            pthread_mutex_unlock(&lock_table_latch);
            pthread_mutex_unlock(&trx_table_latch);
            
            // *ret_lock = *ret;
            // memcpy(ret_lock, ret, L_OBJ_SIZE);
            *ret_lock = ret;
            return 0;
            // return ret;
        }
        
		while(prev_obj != NULL){
			if(prev_obj->mode == 1 ){ //앞에 1개라도 x 있으면 기다리기
				pthread_t tmp_id = pthread_self();
				int d = detect_deadlock(trx_id, prev_obj->owner_id);
                
				if(d == 1){
					printf("deadlock ! \n");

					// int ret = roll_back(trx_id);
                    // if(ret == 0){
                    //     // printf("roll_back fail \n");
                    //     exit(1);
                    // }

					pthread_mutex_unlock(&lock_table_latch);
					pthread_mutex_unlock(&trx_table_latch);
                    return 2;
					// return (void*) 0;
				}
				// pthread_mutex_unlock(&lock_table_latch);
                // printf("s lock sleep\n");
				// pthread_cond_wait(&obj->cv,&trx_table_latch);
				// pthread_mutex_lock(&lock_table_latch);
                // memcpy(ret_lock, obj, L_OBJ_SIZE);
                *ret_lock = obj;
                return 1;
				
			}
			prev_obj = prev_obj->prev;
		}
	}

	if(lock_mode == 1){  //x lock
        if(obj->sentinel->head != NULL && obj->sentinel->head->owner_id == obj->owner_id){ 
            if(obj->sentinel->head->mode == 1){ //이미 x lock을 가지고 있는 상황
                
                lock_t * ret = obj->sentinel->head;
                if(obj != obj->sentinel->head){
                    lock_t * tmp = del_obj(obj);
                }
                // lock_t * tmp = del_obj(obj);
                pthread_mutex_unlock(&lock_table_latch);
                pthread_mutex_unlock(&trx_table_latch);
                // memcpy(ret_lock, ret, L_OBJ_SIZE);
                *ret_lock = ret;
                // ret_lock = ret;
                return 0;
                // return ret;
            }
            else{
                if(obj->sentinel->head->next == NULL || obj->sentinel->head->next->mode == 1 ){ //이미 s lock을 가지고 있는데 같이 s lock을 잡고 있는 trx가 없는 경우
                    lock_t * ret = obj->sentinel->head;
                    printf("chang s -> x \n");
                    // if(obj != obj->sentinel->head){
                    //     lock_t * tmp = del_obj(obj);
                    // }
                    lock_t * tmp = del_obj(obj);
                    obj->sentinel->head->mode = 1;
                    // printf("%s\n",value);
                    memcpy(obj->sentinel->head->old_v, value, VALUE_SIZE);
                    printf("%s\n",obj->sentinel->head->old_v);
                    pthread_mutex_unlock(&lock_table_latch);
                    pthread_mutex_unlock(&trx_table_latch);
                    // memcpy(ret_lock, ret, L_OBJ_SIZE);
                    *ret_lock = ret;
                    return 0;
                    // return ret;
                }
            }
        }
		if(prev_obj != NULL){
			pthread_t tmp_id = pthread_self();
			int d = detect_deadlock(trx_id, prev_obj->owner_id);
			if(d == 1){
				printf("deadlock ! \n");

				// int ret = roll_back(trx_id);
                // if(ret == 0){
                //     // printf("roll_back fail \n");
                //     exit(1);
                // }

				pthread_mutex_unlock(&lock_table_latch);
				pthread_mutex_unlock(&trx_table_latch);
				return 2;
			}
			// pthread_mutex_unlock(&lock_table_latch);
			// pthread_cond_wait(&obj->cv,&trx_table_latch);
			// pthread_mutex_lock(&lock_table_latch);
            // memcpy(ret_lock, obj, L_OBJ_SIZE);
            *ret_lock = obj;
            return 1;
		}
	}

	pthread_mutex_unlock(&lock_table_latch);
	pthread_mutex_unlock(&trx_table_latch);

	if(obj != NULL){
        // memcpy(ret_lock, obj, L_OBJ_SIZE);
        *ret_lock = obj;
        return 0;
		// return obj;
	}

	return -1;
}

int
lock_release(lock_t* lock_obj)
{
    
	// pthread_mutex_lock(&lock_table_latch);
	lock_t * next_obj = NULL;
	int now_mode = lock_obj->mode;

	next_obj = del_obj(lock_obj);
	
	if(next_obj != NULL){
		

		if(now_mode == 0){
			if( now_mode == next_obj->mode ){ //s lock(지운거) - s lock(다음거)
				// printf("s - s\n");
				// pthread_mutex_unlock(&lock_table_latch);
				return 0;
			}
			else{  //s lock - x lock
				if(next_obj->prev == NULL){
					// printf("s - x\n");
                    trx * tar_trx = trx_find(next_obj->owner_id);
                    pthread_mutex_lock(&tar_trx->trx_latch);
					pthread_cond_signal(&next_obj->cv);
                    pthread_mutex_unlock(&tar_trx->trx_latch);
					// pthread_mutex_unlock(&lock_table_latch);
					return 0;
				}
			}
			
		}

		if(now_mode == 1){
			if( now_mode == next_obj->mode ){ //x lock - x lock
				// printf("x - x\n");
				trx * tar_trx = trx_find(next_obj->owner_id);
                pthread_mutex_lock(&tar_trx->trx_latch);
                pthread_cond_signal(&next_obj->cv);
                pthread_mutex_unlock(&tar_trx->trx_latch);
				// pthread_mutex_unlock(&lock_table_latch);
				return 0;
			}
			else{ //x lock - s lock
				// printf("x - s\n");
				while(next_obj != NULL && next_obj->mode != 1){
					trx * tar_trx = trx_find(next_obj->owner_id);
                    // printf("IN lock_lease, trx_id : %d\n",next_obj->owner_id);
                    pthread_mutex_lock(&tar_trx->trx_latch);
                    // printf("IN lock_release, before signal\n");
					pthread_cond_signal(&next_obj->cv);
                    // printf("IN lock_release, pointer : %p\n",next_obj);
                    // printf("IN lock_release, before unlock\n");
                    pthread_mutex_unlock(&tar_trx->trx_latch);
                    // printf("IN lock_release, after unlock\n");
					next_obj = next_obj->next;
				}
				// pthread_mutex_unlock(&lock_table_latch);
				return 0;
			}
		}


	}

	
	return 0;
}

void lock_wait(lock_t * lock_obj, int table_id, pagenum_t leafN){
    // pthread_mutex_lock(&trx_table_latch);
    // printf("IN lock_wait, before trx_find\n");
    trx * tar_trx = trx_find(lock_obj->owner_id);
    // printf("IN lock_wait, after trx_find\n");
    pthread_mutex_lock(&tar_trx->trx_latch);
    // printf("IN lock_wait, trx_id : %d\n",tar_trx->id);
    // printf("IN lock_wait, after trx_latch lock\n");
    pthread_mutex_unlock(&trx_table_latch);
    
    buff_release_page_latch(table_id, leafN);
    pthread_mutex_unlock(&lock_table_latch);
    printf("IN lock_wait, before sleep\n");
    // printf("IN lock_wait, pointer : %p\n",lock_obj);
    pthread_cond_wait(&lock_obj->cv,&tar_trx->trx_latch);
    printf("IN lock_wait, after sleep\n");
}




//trxM

void init_trxM(void){
    trxM = (trxManager*)malloc(sizeof(trxManager));
    trxM->global_trx_id = 1;
    trxM->head_trx = NULL;
}

void print_trxM(void){
    pthread_mutex_lock(&lock_table_latch);
	pthread_mutex_lock(&trx_table_latch);

    printf("IN print_trxM, now global_trx_id : %d\n",trxM->global_trx_id);
    trx * tmp_ptr = trxM->head_trx;

    while(tmp_ptr != NULL){
        printf("trx id : %d\n",tmp_ptr->id);
        lock_t * tmp_ptr2 = tmp_ptr->first_obj;
        tmp_ptr = tmp_ptr->next;
        while(tmp_ptr2 != NULL){
            printf("%d  ",tmp_ptr2->mode);
            // tmp_ptr2 = tmp_ptr2->next;
            tmp_ptr2 = tmp_ptr2->trxnextlock;
        }
        printf("\n\n");
        // tmp_ptr = tmp_ptr->next;
    }

	pthread_mutex_unlock(&lock_table_latch);
	pthread_mutex_unlock(&trx_table_latch);
}

trx * trx_find(int trx_id){
    trx * tmp_ptr = trxM->head_trx;
    if(tmp_ptr == NULL){
        return NULL;
    }
    while(tmp_ptr != NULL){
        if(tmp_ptr->id == trx_id){
            return tmp_ptr;
        }
        tmp_ptr = tmp_ptr->next;
    }
    return NULL;
}

int trx_begin(void){


	pthread_mutex_lock(&trx_table_latch);

    trx * tmp_trx = (trx*)malloc(sizeof(trx));
    
    if(tmp_trx == NULL){
		pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }

    tmp_trx->id = trxM->global_trx_id; //trx 만들기
    tmp_trx->first_obj = NULL;   
    tmp_trx->prev = NULL;
    tmp_trx->next =NULL;
    pthread_mutex_init(&tmp_trx->trx_latch,NULL);


    pthread_mutex_lock(&log_buff_latch);
    tmp_trx->last_LSN = log_buff->global_LSN;
    tmp_trx->prev_LSN = -1;
    log_buff_write(tmp_trx->prev_LSN,tmp_trx->id,LOG_BEG_TYPE, 0, 0, 0, "\0", "\0");
    pthread_mutex_unlock(&log_buff_latch);
    


    trxM->global_trx_id += 1;

    trx * tmp_ptr = trxM->head_trx; //trx를 trxM에 넣기
    
    if(tmp_ptr == NULL){ //trxM에 처음 trx를 넣는 경우
        trxM->head_trx = tmp_trx;
    }

    else{  //trxM에서 linked list 구조를 이용해 마지막에 넣어주는 경우
        while(tmp_ptr->next != NULL){
            tmp_ptr = tmp_ptr->next;
        }
        tmp_ptr->next = tmp_trx;
        tmp_trx->prev = tmp_ptr;
    }
	
	pthread_mutex_unlock(&trx_table_latch);
    
    // print_trxM();


    return tmp_trx->id;
}

int trx_commit(int trx_id){

    pthread_mutex_lock(&lock_table_latch);
	pthread_mutex_lock(&trx_table_latch);

	int ret = 0;
	
    trx * tar_trx = trx_find(trx_id);
    
    
    if(tar_trx == NULL){
        printf("IN trx_commit, tar_trx is null\n");
		pthread_mutex_unlock(&lock_table_latch);
		pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }




    pthread_mutex_lock(&log_buff_latch);
    int64_t tmp_LSN = log_buff->global_LSN;
    log_buff_write(tar_trx->last_LSN, tar_trx->id,LOG_COMM_TYPE, 0, 0, 0, "\0", "\0");
    tar_trx->last_LSN = tmp_LSN;

    // print_log_buff();

    log_buff_flush();

    pthread_mutex_unlock(&log_buff_latch);




    //lock release 전부 해주고 (trx의 lock obj 없애주기)
    lock_t * tmp_ptr = tar_trx->first_obj;
    if(tmp_ptr != NULL){  //obj를 가지고 있다면
        while(tmp_ptr->trxnextlock != NULL){ //trx의 마지막 obj 찾기
        tmp_ptr = tmp_ptr->trxnextlock;
        }

        while(tmp_ptr->trxprevlock != NULL){ //전부 free 해주기
            lock_t * free_obj = tmp_ptr;
            tmp_ptr = tmp_ptr->trxprevlock;
			ret = lock_release(free_obj);
            // print_trxM();
        }
        tar_trx->first_obj = NULL;
		ret = lock_release(tmp_ptr);
    }

    //trxM에서 해당 trx 빼기
    if(tar_trx->prev != NULL){
        tar_trx->prev->next = tar_trx->next;
    }
    else{
        // printf("prev is null\n");  //list의 첫번째라는 것
        if(tar_trx->next != NULL){
            tar_trx->next->prev = NULL;
            trxM->head_trx = tar_trx->next;
        }
        else{  //완전히 빈 리스트가 될때
            trxM->head_trx = NULL;
        }
    }

    if(tar_trx->next != NULL){
        tar_trx->next->prev = tar_trx->prev;
    }
    else{
        // printf("next is null\n");
        if(tar_trx->prev != NULL){  //list의 마지막 요소가 삭제될때
            tar_trx->prev->next = NULL;
        }
        else{  //완전히 빈 리스트가 될때
            trxM->head_trx = NULL;
        }
    }

    free(tar_trx);

	pthread_mutex_unlock(&lock_table_latch);
	pthread_mutex_unlock(&trx_table_latch);


    return trx_id;
}

int add_obj_trx(int trx_id, lock_t * lock_obj){
    trx * tar_trx = trx_find(trx_id);
    if(tar_trx == NULL){
        return 0;
    }
    lock_t * tmp_ptr = tar_trx->first_obj;
    if(tmp_ptr == NULL){
        tar_trx->first_obj = lock_obj;
		tar_trx->first_obj->trxnextlock = NULL;
		tar_trx->first_obj->trxprevlock = NULL;
        return 1;
    }
    while(tmp_ptr->trxnextlock != NULL){
        tmp_ptr = tmp_ptr->trxnextlock;
    }
    tmp_ptr->trxnextlock = lock_obj;
    
	lock_obj->trxprevlock = tmp_ptr;
	lock_obj->trxnextlock = NULL;

    return 1;
}

int roll_back(int trx_id){
    // pthread_mutex_lock(&lock_table_latch);
    // print_trxM();
	pthread_mutex_lock(&trx_table_latch);
    // printf("trx_abort start, trx_id : %d\n",trx_id);
    

	int ret = 0;
    int obj_cnt = 0;
	
    trx * tar_trx = trx_find(trx_id);
    pthread_mutex_unlock(&trx_table_latch);
    
    if(tar_trx == NULL){
		// pthread_mutex_unlock(&lock_table_latch);
		// pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }
    //lock release 전부 해주고 (trx의 lock obj 없애주기)
    // pthread_mutex_unlock(&lock_table_latch);
    // pthread_mutex_unlock(&trx_table_latch);
    lock_t * tmp_ptr = tar_trx->first_obj;
    if(tmp_ptr != NULL){  //obj를 가지고 있다면
        while(tmp_ptr->trxnextlock != NULL){ //trx의 마지막 obj 찾기
        tmp_ptr = tmp_ptr->trxnextlock;
        }

        while(tmp_ptr->trxprevlock != NULL){ //전부 lock_release 해주기
            lock_t * free_obj = tmp_ptr;
            page_t tmp_page;
            obj_cnt += 1;
            //roll_back start
            pagenum_t tmp_pageN = find_leaf(free_obj->sentinel->tid, free_obj->sentinel->rid);
            if(free_obj->mode == 1){
                printf("free_obj->old_v : %s\n",free_obj->old_v);
                if(obj_cnt != 1){
                    buff_get_page_latch(free_obj->sentinel->tid, tmp_pageN);

                    buff_update_read_page(free_obj->sentinel->tid, tmp_pageN,&tmp_page);
                    // obj_cnt = 0;
                    //여기서부터 시작
                    pthread_mutex_lock(&trx_table_latch);

                    pthread_mutex_lock(&log_buff_latch);
                    int64_t tmp_LSN = log_buff->global_LSN;

                    int i;
                    char old_tmp_page_value[VALUE_SIZE];
                    for( i = 0; i < tmp_page.L.header.numKeys; i++){
                        if(tmp_page.L.records[i].key == free_obj->sentinel->rid){
                            tmp_page.L.header.page_LSN = tmp_LSN;
                            // printf("IN trx_abort, before roll_back, leaf value : %s\n", tmp_page.L.records[i].value);
                            memcpy(old_tmp_page_value, tmp_page.L.records[i].value, VALUE_SIZE);
                            memcpy(tmp_page.L.records[i].value, free_obj->old_v ,VALUE_SIZE);
                            break;
                        }
                    }

                    //write compensate log, log의 old_val에 현재 page의 값, new_val에 이전 값(old_v) => redo 할때 일괄 처리 할라고
                    //db_update와는 다르게 buff_update_read_page 이후에 하는 이유는 index (여기선 i) 값을 위에서 계산해줘야 하니까
                    // pthread_mutex_lock(&lock_table_latch);
                    // pthread_mutex_lock(&trx_table_latch);

                    // pthread_mutex_lock(&log_buff_latch);
                    // int64_t tmp_LSN = log_buff->global_LSN;
                    log_buff_write(tar_trx->last_LSN, tar_trx->id,LOG_COM_TYPE, free_obj->sentinel->tid, tmp_pageN, i, old_tmp_page_value, free_obj->old_v);
                    tar_trx->last_LSN = tmp_LSN;
                    pthread_mutex_unlock(&log_buff_latch);

                    // pthread_mutex_unlock(&lock_table_latch);
                    pthread_mutex_unlock(&trx_table_latch);



                    buff_update_write_page(free_obj->sentinel->tid, tmp_pageN,&tmp_page);
                }
            }
            //roll_back end
            tmp_ptr = tmp_ptr->trxprevlock;
            pthread_mutex_lock(&lock_table_latch);
            pthread_mutex_lock(&trx_table_latch);
			ret = lock_release(free_obj);
            pthread_mutex_unlock(&lock_table_latch);
            pthread_mutex_unlock(&trx_table_latch);
            // print_trxM();
        }
        //마지막 obj에 대해 roll back start
        if(tmp_ptr->mode == 1){
            printf("tmp_ptr->old_v : %s\n",tmp_ptr->old_v);
            obj_cnt += 1;
            if(obj_cnt != 1){
                // obj_cnt = 0;
                page_t tmp_page;
                pagenum_t tmp_pageN = find_leaf(tmp_ptr->sentinel->tid, tmp_ptr->sentinel->rid);
                buff_get_page_latch(tmp_ptr->sentinel->tid, tmp_pageN);
                buff_update_read_page(tmp_ptr->sentinel->tid, tmp_pageN, &tmp_page);


                //여기서부터 시작
                pthread_mutex_lock(&trx_table_latch);

                pthread_mutex_lock(&log_buff_latch);
                int64_t tmp_LSN = log_buff->global_LSN;
                
                int i;
                char old_tmp_page_value[VALUE_SIZE];
                for( i = 0; i < tmp_page.L.header.numKeys; i++){
                    if(tmp_page.L.records[i].key == tmp_ptr->sentinel->rid){
                        tmp_page.L.header.page_LSN = tmp_LSN;
                        memcpy(old_tmp_page_value, tmp_page.L.records[i].value, VALUE_SIZE);
                        memcpy(tmp_page.L.records[i].value, tmp_ptr->old_v ,VALUE_SIZE);
                        break;
                    }
                }

                //write compensate log, log의 old_val에 현재 page의 값, new_val에 이전 값(old_v) => redo 할때 일괄 처리 할라고
                //db_update와는 다르게 buff_update_read_page 이후에 하는 이유는 index (여기선 i) 값을 위에서 계산해줘야 하니까
                // pthread_mutex_lock(&lock_table_latch);
                // pthread_mutex_lock(&trx_table_latch);

                // pthread_mutex_lock(&log_buff_latch);
                // int64_t tmp_LSN = log_buff->global_LSN;
                log_buff_write(tar_trx->last_LSN, tar_trx->id, LOG_COM_TYPE, tmp_ptr->sentinel->tid, tmp_pageN, i, old_tmp_page_value, tmp_ptr->old_v);
                tar_trx->last_LSN = tmp_LSN;
                pthread_mutex_unlock(&log_buff_latch);

                // pthread_mutex_unlock(&lock_table_latch);
                pthread_mutex_unlock(&trx_table_latch);


                buff_update_write_page(tmp_ptr->sentinel->tid, tmp_pageN, &tmp_page);
            }
        }
        //마지막 obj에 대해 roll back end
        tar_trx->first_obj = NULL;
        pthread_mutex_lock(&lock_table_latch);
        pthread_mutex_lock(&trx_table_latch);
		ret = lock_release(tmp_ptr);
        pthread_mutex_unlock(&lock_table_latch);
        pthread_mutex_unlock(&trx_table_latch);
    }

    pthread_mutex_lock(&lock_table_latch);
	pthread_mutex_lock(&trx_table_latch);

    pthread_mutex_lock(&log_buff_latch);
    int64_t tmp_LSN = log_buff->global_LSN;
    log_buff_write(tar_trx->last_LSN, tar_trx->id,LOG_COMM_TYPE, 0, 0, 0, "\0", "\0");
    tar_trx->last_LSN = tmp_LSN;

    log_buff_flush();

    pthread_mutex_unlock(&log_buff_latch);


    //trxM에서 해당 trx 빼기
    if(tar_trx->prev != NULL){
        tar_trx->prev->next = tar_trx->next;
    }
    else{
        // printf("prev is null\n");  //list의 첫번째라는 것
        if(tar_trx->next != NULL){
            tar_trx->next->prev = NULL;
            trxM->head_trx = tar_trx->next;
        }
        else{  //완전히 빈 리스트가 될때
            trxM->head_trx = NULL;
        }
    }

    if(tar_trx->next != NULL){
        tar_trx->next->prev = tar_trx->prev;
    }
    else{
        // printf("next is null\n");
        if(tar_trx->prev != NULL){  //list의 마지막 요소가 삭제될때
            tar_trx->prev->next = NULL;
        }
        else{  //완전히 빈 리스트가 될때
            trxM->head_trx = NULL;
        }
    }

    free(tar_trx);

	pthread_mutex_unlock(&lock_table_latch);
	pthread_mutex_unlock(&trx_table_latch);
    return trx_id;
}

//tmp point << 빨리 찾기위해서 써둠
int trx_abort(int trx_id){
    // pthread_mutex_lock(&lock_table_latch);
    // print_trxM();
	pthread_mutex_lock(&trx_table_latch);
    // printf("trx_abort start, trx_id : %d\n",trx_id);
    

	int ret = 0;
    int obj_cnt = 0;
	
    trx * tar_trx = trx_find(trx_id);
    pthread_mutex_unlock(&trx_table_latch);
    
    if(tar_trx == NULL){
		// pthread_mutex_unlock(&lock_table_latch);
		// pthread_mutex_unlock(&trx_table_latch);
        return 0;
    }
    //lock release 전부 해주고 (trx의 lock obj 없애주기)
    // pthread_mutex_unlock(&lock_table_latch);
    // pthread_mutex_unlock(&trx_table_latch);
    lock_t * tmp_ptr = tar_trx->first_obj;
    if(tmp_ptr != NULL){  //obj를 가지고 있다면
        while(tmp_ptr->trxnextlock != NULL){ //trx의 마지막 obj 찾기
        tmp_ptr = tmp_ptr->trxnextlock;
        }

        while(tmp_ptr->trxprevlock != NULL){ //전부 lock_release 해주기
            lock_t * free_obj = tmp_ptr;
            page_t tmp_page;
            obj_cnt += 1;
            //roll_back start
            pagenum_t tmp_pageN = find_leaf(free_obj->sentinel->tid, free_obj->sentinel->rid);
            if(free_obj->mode == 1){
                printf("free_obj->old_v : %s\n",free_obj->old_v);
                // [if(obj_cnt != 1){
                    buff_get_page_latch(free_obj->sentinel->tid, tmp_pageN);

                    buff_update_read_page(free_obj->sentinel->tid, tmp_pageN,&tmp_page);
                    // obj_cnt = 0;
                    //여기서부터 시작
                    pthread_mutex_lock(&trx_table_latch);

                    pthread_mutex_lock(&log_buff_latch);
                    int64_t tmp_LSN = log_buff->global_LSN;

                    int i;
                    char old_tmp_page_value[VALUE_SIZE];
                    for( i = 0; i < tmp_page.L.header.numKeys; i++){
                        if(tmp_page.L.records[i].key == free_obj->sentinel->rid){
                            tmp_page.L.header.page_LSN = tmp_LSN;
                            // printf("IN trx_abort, before roll_back, leaf value : %s\n", tmp_page.L.records[i].value);
                            memcpy(old_tmp_page_value, tmp_page.L.records[i].value, VALUE_SIZE);
                            memcpy(tmp_page.L.records[i].value, free_obj->old_v ,VALUE_SIZE);
                            break;
                        }
                    }

                    //write compensate log, log의 old_val에 현재 page의 값, new_val에 이전 값(old_v) => redo 할때 일괄 처리 할라고
                    //db_update와는 다르게 buff_update_read_page 이후에 하는 이유는 index (여기선 i) 값을 위에서 계산해줘야 하니까
                    // pthread_mutex_lock(&lock_table_latch);
                    // pthread_mutex_lock(&trx_table_latch);

                    // pthread_mutex_lock(&log_buff_latch);
                    // int64_t tmp_LSN = log_buff->global_LSN;
                    log_buff_write(tar_trx->last_LSN, tar_trx->id,LOG_COM_TYPE, free_obj->sentinel->tid, tmp_pageN, i, old_tmp_page_value, free_obj->old_v);
                    tar_trx->last_LSN = tmp_LSN;
                    pthread_mutex_unlock(&log_buff_latch);

                    // pthread_mutex_unlock(&lock_table_latch);
                    pthread_mutex_unlock(&trx_table_latch);



                    buff_update_write_page(free_obj->sentinel->tid, tmp_pageN,&tmp_page);
                // }
            }
            //roll_back end
            tmp_ptr = tmp_ptr->trxprevlock;
            pthread_mutex_lock(&lock_table_latch);
            pthread_mutex_lock(&trx_table_latch);
			ret = lock_release(free_obj);
            pthread_mutex_unlock(&lock_table_latch);
            pthread_mutex_unlock(&trx_table_latch);
            // print_trxM();
        }
        //마지막 obj에 대해 roll back start
        if(tmp_ptr->mode == 1){
            printf("tmp_ptr->old_v : %s\n",tmp_ptr->old_v);
            obj_cnt += 1;
            // if(obj_cnt != 1){
                // obj_cnt = 0;
                page_t tmp_page;
                pagenum_t tmp_pageN = find_leaf(tmp_ptr->sentinel->tid, tmp_ptr->sentinel->rid);
                buff_get_page_latch(tmp_ptr->sentinel->tid, tmp_pageN);
                buff_update_read_page(tmp_ptr->sentinel->tid, tmp_pageN, &tmp_page);


                //여기서부터 시작
                pthread_mutex_lock(&trx_table_latch);

                pthread_mutex_lock(&log_buff_latch);
                int64_t tmp_LSN = log_buff->global_LSN;
                
                int i;
                char old_tmp_page_value[VALUE_SIZE];
                for( i = 0; i < tmp_page.L.header.numKeys; i++){
                    if(tmp_page.L.records[i].key == tmp_ptr->sentinel->rid){
                        tmp_page.L.header.page_LSN = tmp_LSN;
                        memcpy(old_tmp_page_value, tmp_page.L.records[i].value, VALUE_SIZE);
                        memcpy(tmp_page.L.records[i].value, tmp_ptr->old_v ,VALUE_SIZE);
                        break;
                    }
                }

                //write compensate log, log의 old_val에 현재 page의 값, new_val에 이전 값(old_v) => redo 할때 일괄 처리 할라고
                //db_update와는 다르게 buff_update_read_page 이후에 하는 이유는 index (여기선 i) 값을 위에서 계산해줘야 하니까
                // pthread_mutex_lock(&lock_table_latch);
                // pthread_mutex_lock(&trx_table_latch);

                // pthread_mutex_lock(&log_buff_latch);
                // int64_t tmp_LSN = log_buff->global_LSN;
                log_buff_write(tar_trx->last_LSN, tar_trx->id, LOG_COM_TYPE, tmp_ptr->sentinel->tid, tmp_pageN, i, old_tmp_page_value, tmp_ptr->old_v);
                tar_trx->last_LSN = tmp_LSN;
                pthread_mutex_unlock(&log_buff_latch);

                // pthread_mutex_unlock(&lock_table_latch);
                pthread_mutex_unlock(&trx_table_latch);


                buff_update_write_page(tmp_ptr->sentinel->tid, tmp_pageN, &tmp_page);
            // }
        }
        //마지막 obj에 대해 roll back end
        tar_trx->first_obj = NULL;
        pthread_mutex_lock(&lock_table_latch);
        pthread_mutex_lock(&trx_table_latch);
		ret = lock_release(tmp_ptr);
        pthread_mutex_unlock(&lock_table_latch);
        pthread_mutex_unlock(&trx_table_latch);
    }

    pthread_mutex_lock(&lock_table_latch);
	pthread_mutex_lock(&trx_table_latch);

    pthread_mutex_lock(&log_buff_latch);
    int64_t tmp_LSN = log_buff->global_LSN;
    log_buff_write(tar_trx->last_LSN, tar_trx->id,LOG_ROLL_TYPE, 0, 0, 0, "\0", "\0");
    tar_trx->last_LSN = tmp_LSN;

    log_buff_flush();

    pthread_mutex_unlock(&log_buff_latch);


    //trxM에서 해당 trx 빼기
    if(tar_trx->prev != NULL){
        tar_trx->prev->next = tar_trx->next;
    }
    else{
        // printf("prev is null\n");  //list의 첫번째라는 것
        if(tar_trx->next != NULL){
            tar_trx->next->prev = NULL;
            trxM->head_trx = tar_trx->next;
        }
        else{  //완전히 빈 리스트가 될때
            trxM->head_trx = NULL;
        }
    }

    if(tar_trx->next != NULL){
        tar_trx->next->prev = tar_trx->prev;
    }
    else{
        // printf("next is null\n");
        if(tar_trx->prev != NULL){  //list의 마지막 요소가 삭제될때
            tar_trx->prev->next = NULL;
        }
        else{  //완전히 빈 리스트가 될때
            trxM->head_trx = NULL;
        }
    }

    free(tar_trx);

	pthread_mutex_unlock(&lock_table_latch);
	pthread_mutex_unlock(&trx_table_latch);
    return trx_id;
}





//deadlock

void init_graphM(void){
    graphM = (graphManager*)malloc(sizeof(graphManager));
    graphM->head_node = NULL;
    graphM->tail_node = NULL;
}

void init_visitM(void){
    visitM = (visitManager*)malloc(sizeof(visitManager));
    visitM->head = NULL;
    visitM->tail = NULL;

    Gnode * tmp_g = graphM->head_node;
    
    while(tmp_g != NULL){
        visit * tmp_v = (visit*)malloc(sizeof(visit));
        tmp_v->vertex = tmp_g->vertex;
        tmp_v->is_check = 0;
        tmp_v->next = NULL;
        tmp_v->prev = NULL;
        if(visitM->head == NULL){
            visitM->head = tmp_v;
            visitM->tail = tmp_v;
        }
        else{
            visitM->tail->next = tmp_v;
            tmp_v->prev = visitM->tail;
            visitM->tail = tmp_v;
        }
        tmp_g = tmp_g->next;
    }
}

void print_graphM(void){
    pthread_mutex_lock(&graph_latch);
    printf("Start print_graphM\n");
    Gnode * tmp_ptr = graphM->head_node;

    while(tmp_ptr != NULL){
        printf("node id : %d\n [  ",tmp_ptr->vertex);
        for(int i = 0; i < tmp_ptr->used_cnt; i++){
            printf("%d  ",tmp_ptr->edge[i]);
        }
        printf("]\n");
        tmp_ptr = tmp_ptr->next;
    }
    pthread_mutex_unlock(&graph_latch);
}

Gnode * Gnode_find(int tar_vertex){  //graph에서 해당 node 찾아줌
    Gnode * tmp_ptr = graphM->head_node;
    if(tmp_ptr == NULL){
        return NULL;
    }
	
    while(tmp_ptr != NULL){
        if(tmp_ptr->vertex == tar_vertex){
            return tmp_ptr;
        }
        tmp_ptr = tmp_ptr->next;
    }
    return NULL;
}

visit * visit_find(int tar_vertex){  //visitM에서 해당 visit 찾아줌
    visit * tmp_ptr = visitM->head;
    if(tmp_ptr == NULL){
        return NULL;
    }
    while(tmp_ptr != NULL){
        if(tmp_ptr->vertex == tar_vertex){
            return tmp_ptr;
        }
        tmp_ptr = tmp_ptr->next;
    }
    return NULL;
}

Gnode * add_vertex(int tar_vertex){  //graph에 vertex 추가, edge는 add_edge로
    // pthread_mutex_lock(&graph_latch);
	Gnode * tmp = (Gnode *)malloc(sizeof(Gnode));
    tmp->vertex = tar_vertex;
    tmp->edge = NULL;
    tmp->used_cnt = 0;

	tmp->prev = NULL;
	tmp->next = NULL;

	if(graphM->head_node == NULL){
		graphM->head_node = tmp;
		graphM->tail_node = tmp;
        
        // pthread_mutex_unlock(&graph_latch);
        return tmp;
	}

	Gnode * ori_tail = graphM->head_node;
	while(ori_tail->next){
		ori_tail = ori_tail->next;
	}
	ori_tail->next = tmp;
	tmp->prev = ori_tail;
	graphM->tail_node = tmp;

    // pthread_mutex_unlock(&graph_latch);
	return tmp;
}

void add_edge(int src, int dest){ //src가 dest를 기다림, graph에 edge추가, vertex는 add_vertex로
    // pthread_mutex_lock(&graph_latch);
    Gnode * tmp = Gnode_find(src);
    tmp->used_cnt += 1;
    tmp->edge = (int*)realloc(tmp->edge, sizeof(int)*tmp->used_cnt);
    tmp->edge[tmp->used_cnt-1] = dest;
    // pthread_mutex_unlock(&graph_latch);
}

void del_vertex(int tar_vertex){  //graph에서 해당 vertex와 해당 vertex로 들어오는 모든 edge 삭제
    // pthread_mutex_lock(&graph_latch);
    Gnode * tmp = graphM->head_node;
    while(tmp != NULL){  //delete edge
        for(int i = 0; i<tmp->used_cnt; i++){
            if(tmp->edge[i] == tar_vertex){
                if(tmp->used_cnt > 0){
                    tmp->edge[i] = tmp->edge[tmp->used_cnt-1];
                    tmp->used_cnt -= 1;
                }
                tmp->edge = (int *)realloc(tmp->edge,sizeof(int)*tmp->used_cnt);
            }
        }
        tmp = tmp->next;
    }

    Gnode * tar_node = Gnode_find(tar_vertex); //In graph, delete vertex 
    if(tar_node == graphM->head_node && graphM->tail_node){
        graphM->head_node = NULL;
        graphM->tail_node = NULL;
    }
    else if(tar_node == graphM->head_node){
        graphM->head_node = tar_node->next;
        if(tar_node->next){
            tar_node->next->prev = NULL;
        }
    }
    else if(tar_node == graphM->tail_node){
        graphM->tail_node = tar_node->prev;
        if(tar_node->prev){
            tar_node->prev->next = NULL;
        }
    }
    else{
        tar_node->prev->next = tar_node->next;
        tar_node->next->prev = tar_node->prev;
    }
    tar_node->next = NULL;
    tar_node->prev = NULL;
    // pthread_mutex_unlock(&graph_latch);
}


void Genqueue( int edge ) {  //graph에서 cycle 체크 할때 쓸 queue의 push
    
    if (GqueueLen == 0) {
        Gqueue = (int*)malloc(sizeof(int));
        GqueueLen += 1;
        Gqueue[GqueueLen-1] = edge;
    }

    else {
        GqueueLen += 1;
        Gqueue = (int*)realloc(Gqueue,sizeof(int)*GqueueLen);
        Gqueue[GqueueLen-1] = edge;
    }
}

int Gdequeue( void ) {  //graph에서 cycle 체크 할때 쓸 queue의 pop
    int ret_i = Gqueue[0];
    for(int i=0; i < (GqueueLen-1); i++){
        Gqueue[i] = Gqueue[i+1];
    }
    Gqueue[GqueueLen-1] = 0;
    GqueueLen -= 1;
    Gqueue = (int*)realloc(Gqueue,sizeof(int)*GqueueLen);

    return ret_i;
}


int check_cycle( int tar_vertex ){  //graph의 cycle 유무 체크, 해당 vertex로 돌아오는 사이클이 있는지만 확인
    // pthread_mutex_lock(&graph_latch);
    // pthread_mutex_lock(&visit_latch);
    // pthread_mutex_lock(&gqueue_latch);
    init_visitM();
    GqueueLen = 0;
    Genqueue(tar_vertex);
	
    while( GqueueLen != 0 ){
        int tmp_len = GqueueLen;
        for(int i = 0; i<tmp_len; i++){
            int tmp = Gdequeue();
            visit * tmp_v = visit_find(tmp);
            tmp_v->is_check = 1;
            Gnode * tmp_node = Gnode_find(tmp);
            for(int j = 0; j < tmp_node->used_cnt; j++){
                if(tmp_node->edge[j] == tar_vertex){
                    // pthread_mutex_unlock(&gqueue_latch);
                    // pthread_mutex_unlock(&visit_latch);
                    // pthread_mutex_unlock(&graph_latch);
					free(Gqueue);
                    return 1;
                }
                visit * tmp_check = visit_find(tmp_node->edge[j]);
                if(tmp_check != NULL && tmp_check->is_check == 0){
                    Genqueue(tmp_node->edge[j]);
                }
            }
        }
    }
    // pthread_mutex_unlock(&gqueue_latch);
    // pthread_mutex_unlock(&visit_latch);
    // pthread_mutex_unlock(&graph_latch);
	free(Gqueue);
    return 0;
}

int detect_deadlock(int src_trx, int dest_trx){
	pthread_mutex_lock(&graph_latch);
	Gnode * v = Gnode_find(src_trx);

	if(v == NULL){
		v = add_vertex(src_trx);
	}
	add_edge(src_trx, dest_trx);
	
	int ret = check_cycle(src_trx);

	if(ret == 1){
		del_vertex(src_trx);
		pthread_mutex_unlock(&graph_latch);
		return 1;
	}
	pthread_mutex_unlock(&graph_latch);
	return 0;
}




//start recovery
void init_recoveryM(void){
    recovM = (recoveryManager *)malloc(sizeof(recoveryManager));
    recovM->head = NULL;
    recovM->log_cnt = 0;
    recovM->redo_head = NULL;
    recovM->undo_head = NULL;
    recovM->winner_cnt = 0;
    recovM->winner_entry = NULL;
    recovM->loser_cnt = 0;
    recovM->loser_entry = NULL;
    // printf("in init recovM : %d %p\n",recovM->head, recovM->head);
    
}

r_trx * r_trx_begin(int trx_id){
    r_trx * tmp_entry = (r_trx *) malloc(sizeof(r_trx));
    tmp_entry->next = NULL;
    tmp_entry->id = trx_id;
    tmp_entry->type = LOSER_TRX;
    tmp_entry->last_LSN = -1;


    if(recovM->head == NULL){
        recovM->head = tmp_entry;
    }
    
    
    else{
        r_trx * tmp_ptr = recovM->head;
        while(tmp_ptr->next != NULL){
            tmp_ptr = tmp_ptr->next;
        }
        tmp_ptr->next = tmp_entry;
    }

    return tmp_entry;
}

r_trx * r_trx_find(int trx_id){
    r_trx * tmp_ptr = recovM->head;
    if(tmp_ptr == NULL){
        return NULL;
    }
    while(tmp_ptr != NULL){
        if(tmp_ptr->id == trx_id){
            return tmp_ptr;
        }
        tmp_ptr = tmp_ptr->next;
    }
    return NULL;
}

void print_recovM(void){
    int idx = 1;
    r_trx * tmp_ptr = recovM->head;
    if(tmp_ptr == NULL){
        printf("recovM is empty\n");
    }
    while(tmp_ptr != NULL){
        printf("%dst recovM block, id : %d, winner type : %d, last_LSN : %d\n", idx, tmp_ptr->id, tmp_ptr->type, tmp_ptr->last_LSN);
        tmp_ptr = tmp_ptr->next;
        idx += 1;
    }
    printf("\n");

    re_entry * tmp_ptr2 = recovM->redo_head;
    idx = 1;
    if(tmp_ptr2 == NULL){
        printf("recovM's redo entry is empty\n");
    }
    while(tmp_ptr2 != NULL){
        printf("%dst redo entry log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d  \n", idx,tmp_ptr2->content.BCR.log_size, tmp_ptr2->content.COM.LSN, tmp_ptr2->content.COM.prev_LSN, tmp_ptr2->content.COM.trx_id, tmp_ptr2->content.COM.type );
        if(tmp_ptr2->content.BCR.type == LOG_UPD_TYPE || tmp_ptr2->content.BCR.type == LOG_COM_TYPE ){
            printf("%dst redo entry tid : %d, pag_num: %d, offset : %d, data_length : %d, old_img : %s, new_img : %s  \n", idx,tmp_ptr2->content.COM.tid, tmp_ptr2->content.COM.page_num, tmp_ptr2->content.COM.offset, tmp_ptr2->content.COM.data_length, tmp_ptr2->content.COM.old_img, tmp_ptr2->content.COM.new_img );
        }
        // printf("%dst redo entry tid : %d, pag_num: %d, offset : %d, data_length : %d, old_img : %s, new_img : %s  \n", idx,tmp_ptr2->content.COM.tid, tmp_ptr2->content.COM.page_num, tmp_ptr2->content.COM.offset, tmp_ptr2->content.COM.data_length, tmp_ptr2->content.COM.old_img, tmp_ptr2->content.COM.new_img );
        tmp_ptr2 = tmp_ptr2->next;
        idx += 1;
    }
    printf("\n");

    un_entry * tmp_ptr3 = recovM->undo_head;
    idx = 1;
    if(tmp_ptr3 == NULL){
        printf("recovM's undo entry is empty\n");
    }
    while(tmp_ptr3 != NULL){
        printf("%dst undo entry log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d  \n", idx,tmp_ptr3->content.COM.log_size, tmp_ptr3->content.COM.LSN, tmp_ptr3->content.COM.prev_LSN, tmp_ptr3->content.COM.trx_id, tmp_ptr3->content.COM.type );
        if(tmp_ptr3->content.BCR.type == LOG_UPD_TYPE || tmp_ptr3->content.BCR.type == LOG_COM_TYPE ){
            printf("%dst undo entry tid : %d, pag_num: %d, offset : %d, data_length : %d, old_img : %s, new_img : %s, next_nudo_LSN : %d\n", idx,tmp_ptr3->content.COM.tid, tmp_ptr3->content.COM.page_num, tmp_ptr3->content.COM.offset, tmp_ptr3->content.COM.data_length, tmp_ptr3->content.COM.old_img, tmp_ptr3->content.COM.new_img, tmp_ptr3->content.COM.next_undo_LSN );
        }
        tmp_ptr3 = tmp_ptr3->next;
        idx += 1;
    }
    printf("\n");
}

void add_redo_entry(log_record_t * content){
    re_entry * tmp_entry = (re_entry*)malloc(sizeof(re_entry));
    tmp_entry->next = 0;
    memcpy(&tmp_entry->content, content, sizeof(log_record_t));
    
    re_entry * tmp_ptr = recovM->redo_head;
    if(tmp_ptr == NULL){
        recovM->redo_head = tmp_entry;
    }
    else{
        re_entry * tmp_ptr = recovM->redo_head;
        while(tmp_ptr->next != NULL){
            tmp_ptr = tmp_ptr->next;
        }
        tmp_ptr->next = tmp_entry;
    }
}

int * who_is_winner(void){
    if(recovM->winner_cnt == 0){
        return 0;
    }
    

    int * tmp_entry = (int *)malloc(sizeof(int)*recovM->winner_cnt);
    int i = 0;

    r_trx * tmp_ptr = recovM->head;

    while(tmp_ptr != NULL){
        if(tmp_ptr->type == WINNER_TRX){
            tmp_entry[i] = tmp_ptr->id;
            i+=1;
        }
        tmp_ptr = tmp_ptr->next;
    }
    

    return tmp_entry;
}

int * who_is_loser(void){
    if(recovM->loser_cnt == 0){
        return 0;
    }
    

    int * tmp_entry = (int *)malloc(sizeof(int)*recovM->loser_cnt);
    int i = 0;

    r_trx * tmp_ptr = recovM->head;

    while(tmp_ptr != NULL){
        if(tmp_ptr->type == LOSER_TRX){
            tmp_entry[i] = tmp_ptr->id;
            i+=1;
        }
        tmp_ptr = tmp_ptr->next;
    }
    

    return tmp_entry;
}

//다시
void add_undo_entry(log_record_t * content){
    un_entry * tmp_entry = (un_entry*)malloc(sizeof(un_entry));
    tmp_entry->prev = 0;
    tmp_entry->next = 0;
    memcpy(&tmp_entry->content, content, sizeof(log_record_t));
    
    un_entry * tmp_ptr = recovM->undo_head;
    if(tmp_ptr == NULL){ //처음 들어갈때
        recovM->undo_head = tmp_entry;
        return;
    }
    else{
        if(recovM->undo_head->content.BCR.LSN < tmp_entry->content.BCR.LSN){ //처음에 들어가는 경우
            tmp_entry->next = recovM->undo_head;
            recovM->undo_head = tmp_entry;
            return;
        }
        un_entry * tmp_ptr = recovM->undo_head;
        while(tmp_ptr->next != NULL){ //중간에 끼어 들어가는 경우
            if(tmp_ptr->next->content.BCR.LSN < tmp_entry->content.BCR.LSN){
                tmp_entry->prev = tmp_ptr;
                tmp_entry->next = tmp_ptr->next;
                tmp_ptr->next = tmp_entry;
                break;
            }
            tmp_ptr = tmp_ptr->next;
        }
        if(tmp_ptr->content.BCR.LSN < tmp_entry->content.BCR.LSN){ //마지막 놈이랑 비교 했을 때 큰 경우
            tmp_entry->prev = tmp_ptr->prev;
            tmp_entry->next = tmp_ptr;
            return;
        }
        tmp_ptr->next = tmp_entry; //마지막 놈보다도 작은 경우
        tmp_entry->prev = tmp_ptr;
    }
}

un_entry * del_undo_entry(void){
    un_entry * tmp_entry = recovM->undo_head;
    if(tmp_entry == NULL){
        // printf("del_Undo_entry is null?\n");
        return NULL;
    }
    if(recovM->undo_head->next == NULL){
        recovM->undo_head = NULL;
    }
    else{
        recovM->undo_head->next->prev = NULL;
        recovM->undo_head = recovM->undo_head->next;
    }
    return tmp_entry;
}


void recov_analysis(void){
    int ret = 0;
    off_t offset = 0;

    log_record_t tmp_record;
    tmp_record.BCR.type = 0;

    r_trx * tmp_ptr = NULL;

    fprintf(log_msg_file, "[ANALYSIS] Analysis pass start \n");


//analysis winner & loser
    while(ret != -1){
        ret = log_buff_search_record(log_file, offset, &tmp_record);
        if(ret == -2){
            printf("log_file is empty\n");
            fprintf(log_msg_file, "[ANALYSIS] log_file is empty and analysis fail\n");
            return ;
        }
        // printf("after search_record, LSN : %d, trx_id : %d\n",tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
        if(tmp_record.BCR.type == LOG_BEG_TYPE ){
            tmp_ptr = r_trx_begin(tmp_record.BCR.trx_id);
            // printf("IN analysis, tmp_ptr id : %d, last_LSN : %d\n", tmp_ptr->id, tmp_ptr->last_LSN);
            tmp_ptr->last_LSN = offset;
            offset += LOG_HEADER_SIZE;
            // printf("after trx_begin, id : %d, type : %d, last_LSN : %d\n",tmp_ptr->id, tmp_ptr->type, tmp_ptr->last_LSN);
            // printf("after trx_begin, head's id : %d, type : %d, last_LSN : %d\n",recovM->head->id, recovM->head->type, recovM->head->last_LSN);
            
            recovM->loser_cnt += 1;
        }
        if( tmp_record.BCR.type == LOG_COMM_TYPE || tmp_record.BCR.type == LOG_ROLL_TYPE){
            tmp_ptr = r_trx_find(tmp_record.BCR.trx_id);
            tmp_ptr->type = WINNER_TRX;
            tmp_ptr->last_LSN = offset;
            offset += LOG_HEADER_SIZE;

            recovM->loser_cnt -= 1;
            recovM->winner_cnt += 1;
        }
        
        if(tmp_record.BCR.type == LOG_UPD_TYPE){
            tmp_ptr = r_trx_find(tmp_record.BCR.trx_id);
            tmp_ptr->last_LSN = offset;
            offset += LOG_UPDR_SIZE;
        }
        if(tmp_record.BCR.type == LOG_COM_TYPE){
            tmp_ptr = r_trx_find(tmp_record.BCR.trx_id);
            tmp_ptr->last_LSN = offset;
            offset += LOG_COMR_SIZE;
        }
        // printf("now offset : %d\n",offset);
    }

    recovM->winner_entry = who_is_winner();
    recovM->loser_entry = who_is_loser();
    fprintf(log_msg_file, "[ANALYSIS] Analysis success. ");
    fprintf(log_msg_file, "Winner:");
    for(int j = 0; j < recovM->winner_cnt; j++){
        fprintf(log_msg_file, " %d",recovM->winner_entry[j]);
        printf("%dst winner trx : %d\n",j+1,recovM->winner_entry[j]);
    }
    fprintf(log_msg_file, ", Loser:");
    for(int j = 0; j < recovM->loser_cnt; j++){
        fprintf(log_msg_file, " %d",recovM->loser_entry[j]);
        printf("%dst loser trx : %d\n",j+1,recovM->loser_entry[j]);
    }
    fprintf(log_msg_file, "\n");
}


void recov_redo(int flag, int log_num){
    int ret = 0;
    off_t offset = 0;
    int count = 0;

    if(flag != 1){
        count = log_num+1;
    }

    log_record_t tmp_record;
    tmp_record.BCR.type = 0;

    r_trx * tmp_ptr = NULL;

    printf("\nREDO START\n");
    fprintf(log_msg_file , "[REDO] Redo pass start \n");

// //make redo_entry
    while(ret != -1){
        ret = log_buff_search_record(log_file, offset, &tmp_record);
        if(ret == -2){
            fprintf(log_msg_file, "[REDO] Redo pass end \n");
            printf("\nlog_file is empty and REDO END\n");
            return ;
        }
        
        add_redo_entry(&tmp_record);

        if( tmp_record.BCR.type == LOG_BEG_TYPE|| tmp_record.BCR.type == LOG_COMM_TYPE || tmp_record.BCR.type == LOG_ROLL_TYPE){
            offset += LOG_HEADER_SIZE;
        }
        
        if(tmp_record.BCR.type == LOG_UPD_TYPE){
            offset += LOG_UPDR_SIZE;
        }
        if(tmp_record.BCR.type == LOG_COM_TYPE){
            offset += LOG_COMR_SIZE;
        }
    }
    print_recovM();

    re_entry * tmp_ptr2 = recovM->redo_head;
    if(tmp_ptr2 == NULL){
        printf("\nREDO END\n");
        fprintf(log_msg_file, "[REDO] Redo pass end \n");
        return;
    }
    while(tmp_ptr2 != NULL && count != log_num){
        count += 1;
        memcpy(&tmp_record, &tmp_ptr2->content, sizeof(log_record_t));
        if( tmp_record.BCR.type == LOG_BEG_TYPE ){
            tmp_ptr2 = tmp_ptr2->next;
            fprintf(log_msg_file , "LSN %lu [BEGIN] Transaction id %d \n", tmp_record.BCR.LSN+tmp_record.BCR.log_size , tmp_record.BCR.trx_id);
            printf("LSN : %d, BEGIN, trx_id : %d\n", tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
            continue;
        }
        if( tmp_record.BCR.type == LOG_COMM_TYPE ){
            fprintf(log_msg_file , "LSN %lu [COMMIT] Transaction id %d \n", tmp_record.BCR.LSN+tmp_record.BCR.log_size , tmp_record.BCR.trx_id);
            printf("LSN : %d, COMMIT, trx_id : %d\n", tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
            tmp_ptr2 = tmp_ptr2->next;
            continue;
        }
        if( tmp_record.BCR.type == LOG_ROLL_TYPE){
            fprintf(log_msg_file , "LSN %lu [ROLLBACK] Transaction id %d \n", tmp_record.BCR.LSN+tmp_record.BCR.log_size , tmp_record.BCR.trx_id);
            printf("LSN : %d, ROLLBACK, trx_id : %d\n", tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
            tmp_ptr2 = tmp_ptr2->next;
            continue;
        }

        char tmp_table_name[10] = "DATA";
        char tmp_table_num[5];
        sprintf(tmp_table_num,"%d",tmp_record.UPD.tid);
        for(int k = 0; k < strlen(tmp_table_num); k++){
            tmp_table_name[4+k] = tmp_table_num[k];
        }
        tmp_table_name[4+strlen(tmp_table_num)] = '\0';
        
        // exit(1);

        int tmp_tid = open_table(tmp_table_name);
        // print_tree(1);

        page_t tmp_page;
        memset(&tmp_page, 0, PAGE_SIZE);
        buff_read_page_no_lock(tmp_tid, tmp_record.UPD.page_num, &tmp_page);
        printf("consder redo : page LSN : %d, record_LSN :  %d\n",tmp_page.L.header.page_LSN, tmp_record.UPD.LSN);
        if(tmp_page.L.header.page_LSN >= tmp_record.UPD.LSN){
            fprintf(log_msg_file, "LSN %lu [CONSIDER REDO] Transaction id %d \n", tmp_record.UPD.LSN+tmp_record.UPD.log_size , tmp_record.UPD.trx_id);
            printf("LSN : %d, CONSIDER_REDO, trx_id : %d\n", tmp_record.UPD.LSN, tmp_record.UPD.trx_id);
            // printf("pag lsn : %d, record ls : %d\n", tmp_page.L.header.page_LSN, tmp_record.UPD.LSN);
            tmp_ptr2 = tmp_ptr2->next;
            // exit(1);
            continue;
        }
        
        tmp_page.L.header.page_LSN = tmp_record.UPD.LSN;
        int tmp_idx = (tmp_record.UPD.offset-136)/128;
        memcpy(&tmp_page.L.records[tmp_idx].value, &tmp_record.UPD.new_img, VALUE_SIZE);
        // printf("redo,tmp_record.page_num : %d, tmp_record.new_img : %s\n",tmp_record.UPD.page_num, tmp_record.UPD.new_img);
        buff_write_page_no_lock(tmp_tid, tmp_record.UPD.page_num, &tmp_page);
        
        // print_tree2(1);
        close_table(tmp_record.UPD.tid);
        
        if(tmp_record.UPD.type == LOG_UPD_TYPE){
            fprintf(log_msg_file, "LSN %lu [UPDATE] Transaction id %d redo apply \n", tmp_record.UPD.LSN+tmp_record.UPD.log_size , tmp_record.UPD.trx_id);
            printf("LSN : %d, UPDATE, trx_id : %d\n", tmp_record.UPD.LSN, tmp_record.UPD.trx_id);
        }

        if(tmp_record.UPD.type == LOG_COM_TYPE){
            fprintf(log_msg_file, "LSN %lu [CLR] next undo lsn %lu \n", tmp_record.COM.LSN+tmp_record.COM.log_size , tmp_record.COM.next_undo_LSN);
            printf("LSN : %d, COMPENSATE, trx_id : %d\n", tmp_record.UPD.LSN, tmp_record.UPD.trx_id);  
        }
        tmp_ptr2 = tmp_ptr2->next;
        // exit(1);
    }
    if(count == log_num){
        return;
    }
    printf("\nREDO END\n");
    fprintf(log_msg_file, "[REDO] Redo pass end \n");
    printf("\n");

}

//다다시
void recov_undo(int flag, int log_num){
    int ret = 0;
    off_t offset = 0;
    log_record_t tmp_record;
    int count = 0;
    if(flag != 2){
        count = log_num+1;
    }

    printf("\nUNDO START\n");
    fprintf(log_msg_file , "[UNDO] Undo pass start \n");

    // recovM->loser_entry = who_is_loser();
    

    for(int i = 0; i < recovM->loser_cnt; i++){
        r_trx * tmp_rt = r_trx_find(recovM->loser_entry[i]);
        // printf("last_LSN : %d\n",tmp_rt->last_LSN);
        ret = log_buff_search_record(log_file, tmp_rt->last_LSN, &tmp_record);
        add_undo_entry(&tmp_record);
    }
    
    // print_recovM();

    un_entry * tmp_entry = del_undo_entry();
    

    while(tmp_entry != NULL && count != log_num){
        count += 1;
        // print_recovM();
        int64_t tmp_prev_LSN = tmp_entry->content.BCR.prev_LSN;
        int64_t tmp_LSN;

        r_trx * tar_rtrx = NULL;

        if(tmp_entry->content.BCR.type == LOG_UPD_TYPE){
            // memset(&tmp_record, 0, LOG_COMR_SIZE);
            // memcpy(&tmp_record, &tmp_entry->content, LOG_COMR_SIZE);

            tmp_LSN = log_buff->global_LSN;
            tar_rtrx = r_trx_find(tmp_entry->content.BCR.trx_id);

            int32_t tmp_i = -1;
            // if(tmp_reocord.BCR.type == LOG_UPD_TYPE){
            tmp_i = (tmp_entry->content.UPD.offset-136)/128;
            log_buff_write(tar_rtrx->last_LSN, tar_rtrx->id, LOG_COM_TYPE, tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, tmp_i, tmp_entry->content.UPD.new_img, tmp_entry->content.UPD.old_img);
            tar_rtrx->last_LSN = tmp_LSN;
            // }
            page_t tmp_page;
            memset(&tmp_page, 0, PAGE_SIZE);
            buff_read_page_no_lock(tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, &tmp_page);
            fprintf(log_msg_file, "LSN %lu [UPDATE] Transaction id %d undo apply \n", tmp_entry->content.UPD.LSN+tmp_entry->content.UPD.log_size, tmp_entry->content.UPD.trx_id);
            memcpy(tmp_page.L.records[tmp_i].value, tmp_entry->content.UPD.old_img, VALUE_SIZE);
            buff_write_page_no_lock(tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, &tmp_page);
        }

        if(tmp_entry->content.BCR.type == LOG_COM_TYPE){
            // memset(&tmp_record, 0, LOG_COMR_SIZE);
            // memcpy(&tmp_record, &tmp_entry->content, LOG_COMR_SIZE);

            tmp_LSN = log_buff->global_LSN;
            tar_rtrx = r_trx_find(tmp_entry->content.BCR.trx_id);
            // printf("tar_rtrx id : %d\n", tar_rtrx->id);

            if(tmp_entry->content.COM.next_undo_LSN != -1){
                ret = log_buff_search_record(log_file, tmp_entry->content.COM.next_undo_LSN, &tmp_entry);

                int32_t tmp_i = -1;
                // if(tmp_reocord.BCR.type == LOG_UPD_TYPE){
                tmp_i = (tmp_entry->content.UPD.offset-136)/128;
                log_buff_write(tar_rtrx->last_LSN, tar_rtrx->id, LOG_COM_TYPE, tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, tmp_i, tmp_entry->content.UPD.new_img, tmp_entry->content.UPD.old_img);
                tar_rtrx->last_LSN = tmp_LSN;
                // }
                page_t tmp_page;
                memset(&tmp_page, 0, PAGE_SIZE);
                buff_read_page_no_lock(tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, &tmp_page);
                memcpy(tmp_page.L.records[tmp_i].value, tmp_entry->content.UPD.old_img, VALUE_SIZE);
                fprintf(log_msg_file, "LSN %lu [UPDATE] Transaction id %d undo apply \n", tmp_entry->content.UPD.LSN+tmp_entry->content.UPD.log_size, tmp_entry->content.UPD.trx_id);
                // print_log_buff();
                buff_write_page_no_lock(tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, &tmp_page);
                // print_log_buff();
            }

        }

        log_record_t tmp_prev_LSN_record;
        memset(&tmp_prev_LSN_record, 0, LOG_COMR_SIZE);
        ret = log_buff_search_record(log_file, tmp_prev_LSN, &tmp_prev_LSN_record);

        if(tmp_entry->content.COM.type == LOG_UPD_TYPE && tmp_prev_LSN_record.COM.type == LOG_UPD_TYPE ){
            ret = log_buff_search_record(log_file, tmp_prev_LSN, &tmp_record);
            add_undo_entry(&tmp_record);
        }
        else if(tmp_entry->content.COM.type == LOG_COM_TYPE && tmp_entry->content.COM.next_undo_LSN != -1){
            ret = log_buff_search_record(log_file, tmp_LSN, &tmp_record);
            add_undo_entry(&tmp_record);
        }
        else{
            log_buff_write(tar_rtrx->last_LSN, tar_rtrx->id, LOG_ROLL_TYPE, 0, 0, 0, "\0", "\0");
        }
        // print_recovM();
        tmp_entry = del_undo_entry();
        // if(tmp_entry != NULL){
        //     printf("tmp_entry LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", tmp_entry->content.UPD.LSN,tmp_entry->content.UPD.prev_LSN, tmp_entry->content.UPD.trx_id, tmp_entry->content.UPD.type );
        // }
        // print_recovM();
    }
    if(count == log_num){
        return;
    }
    
    printf("\nUNDO END\n");
    fprintf(log_msg_file, "[UNDO] Undo pass end \n");
}