#include "bpt.h"

page_t header;
tManager * tableM;
buffer * buff;
log_buffer * log_buff;
int log_file;

int open_table(char* pathname){
    int ret = open_table_sec(pathname);

    return ret;
}

int close_table(int table_id){
    int ret = close_table_sec(table_id);
    
    return ret;
}

// int init_db(int num_buf, char * log_path){
//     int ret = init_buff(num_buf);
//     ret = init_log_buff(num_buf, log_path);
//     printf("Init log_buff succ\n");

//     // void log_buff_write(int trx_id, int type, int table_id, pagenum_t page_NUM, int index, char * old_img, char * new_img)
//     char oldv[5] = "old";
//     char newv[5] = "new";
//     print_log_buff();
//     log_buff_write(1, 0, 1, 0, 0, 0, 0);
//     print_log_buff();
//     log_buff_write(1, 1, 1, 99, 0, oldv, newv);
//     print_log_buff();
//     log_buff_write(1, 2, 1, 0, 0, 0, 0);
//     print_log_buff();
//     log_buff_write(1, 4, 1, 100, 1, newv, oldv);
//     print_log_buff();
//     log_buff_write(1, 3, 1, 0, 0, 0, 0);
//     print_log_buff();

//     off_t tmp_off = lseek(log_file, 0, SEEK_END);
//     printf("logfile : fd : %d, size : %d\n", log_file, tmp_off);
    
//     log_buff_flush();
//     tmp_off = lseek(log_file, 0, SEEK_END);
//     printf("after flush logfile : fd : %d, size : %d\n", log_file, tmp_off);
//     print_log_buff();

//     log_record_t dest;
//     memset(&dest, 0, LOG_COMR_SIZE);
    
//     off_t cur_off = 0;
//     lseek(log_file, cur_off, SEEK_SET);
//     read(log_file, &dest, LOG_HEADER_SIZE);
//     cur_off += LOG_HEADER_SIZE;
//     printf("first record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );
    
//     lseek(log_file, cur_off, SEEK_SET);
//     read(log_file, &dest, LOG_UPDR_SIZE);
//     cur_off += LOG_UPDR_SIZE;
//     printf("second record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s\n", dest.UPD.log_size, dest.UPD.LSN, dest.UPD.prev_LSN, dest.UPD.trx_id, dest.UPD.type, dest.UPD.tid, dest.UPD.page_num, dest.UPD.offset, dest.UPD.data_length, dest.UPD.old_img, dest.UPD.new_img );

//     lseek(log_file, cur_off, SEEK_SET);
//     read(log_file, &dest, LOG_HEADER_SIZE);
//     cur_off += LOG_HEADER_SIZE;
//     printf("third record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );


//     lseek(log_file, cur_off, SEEK_SET);
//     read(log_file, &dest, LOG_COMR_SIZE);
//     cur_off += LOG_COMR_SIZE;
//     printf("forth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s, nex_undo : %d\n", dest.COM.log_size, dest.COM.LSN, dest.COM.prev_LSN, dest.COM.trx_id, dest.COM.type, dest.COM.tid, dest.COM.page_num, dest.COM.offset, dest.COM.data_length, dest.COM.old_img, dest.COM.new_img, dest.COM.next_undo_LSN );

//     lseek(log_file, cur_off, SEEK_SET);
//     read(log_file, &dest, LOG_HEADER_SIZE);
//     cur_off += LOG_HEADER_SIZE;
//     printf("fifth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );

    

//     return ret;
// }


int init_db(int num_buf, char * log_path){
    int ret = init_buff(num_buf);
    ret = init_log_buff(num_buf, log_path);
    printf("Init log_buff succ\n");
    printf("After init_buf, global lsn : %d, prev lsn : %d\n",log_buff->global_LSN, log_buff->global_prev_LSN);

    // log_buff_read_all_file(){
    // exit(1);

    // void log_buff_write(int trx_id, int type, int table_id, pagenum_t page_NUM, int index, char * old_img, char * new_img)
    char oldv[5] = "a";
    char newv[5] = "b";
    // print_log_buff();

    //type == begin
    int64_t tmp_global_LSN = -1;  //?????? LSN
    int64_t tmp_global_LSN2 = log_buff->global_LSN; //?????? LSN
    log_buff_write(tmp_global_LSN, 1, 0, 1, 0, 0, 0, 0);
    // print_log_buff();

    //type == update
    tmp_global_LSN = tmp_global_LSN2; //0
    tmp_global_LSN2 = log_buff->global_LSN; //28
    log_buff_write(tmp_global_LSN, 1, 1, 1, 99, 0, oldv, newv);
    tmp_global_LSN = tmp_global_LSN2; 
    tmp_global_LSN2 = log_buff->global_LSN; 
    log_buff_write(tmp_global_LSN, 1, 1, 1, 99, 1, oldv, newv);
    // print_log_buff();

    // type == commit
    // log_buff_write(log_buff->global_LSN, 1, 2, 1, 0, 0, 0, 0);
    // print_log_buff();
    //

    // type == compensate
    tmp_global_LSN = tmp_global_LSN2; 
    tmp_global_LSN2 = log_buff->global_LSN; 
    log_buff_write(tmp_global_LSN, 1, 4, 1, 99, 1, newv, oldv);
    tmp_global_LSN = tmp_global_LSN2; 
    tmp_global_LSN2 = log_buff->global_LSN; 
    log_buff_write(tmp_global_LSN, 1, 4, 1, 99, 0, newv, oldv);
    // print_log_buff();

    // type == rollback
    tmp_global_LSN = tmp_global_LSN2; 
    tmp_global_LSN2 = log_buff->global_LSN; 
    log_buff_write(tmp_global_LSN, 1, 3, 1, 0, 0, 0, 0);
    // print_log_buff();

    off_t tmp_off = lseek(log_file, 0, SEEK_END);
    printf("logfile : fd : %d, size : %d\n", log_file, tmp_off);
    
    log_buff_flush();
    tmp_off = lseek(log_file, 0, SEEK_END);
    printf("after flush logfile : fd : %d, size : %d\n", log_file, tmp_off);
    print_log_buff();


    log_buff_read_all_file();
    exit(1);

    // log_record_t dest;
    // memset(&dest, 0, LOG_COMR_SIZE);
    
    // off_t cur_off = 0;
    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_HEADER_SIZE);
    // cur_off += LOG_HEADER_SIZE;
    // printf("first record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );
    
    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_UPDR_SIZE);
    // cur_off += LOG_UPDR_SIZE;
    // printf("second record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s\n", dest.UPD.log_size, dest.UPD.LSN, dest.UPD.prev_LSN, dest.UPD.trx_id, dest.UPD.type, dest.UPD.tid, dest.UPD.page_num, dest.UPD.offset, dest.UPD.data_length, dest.UPD.old_img, dest.UPD.new_img );

    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_HEADER_SIZE);
    // cur_off += LOG_HEADER_SIZE;
    // printf("third record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );


    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_COMR_SIZE);
    // cur_off += LOG_COMR_SIZE;
    // printf("forth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s, nex_undo : %d\n", dest.COM.log_size, dest.COM.LSN, dest.COM.prev_LSN, dest.COM.trx_id, dest.COM.type, dest.COM.tid, dest.COM.page_num, dest.COM.offset, dest.COM.data_length, dest.COM.old_img, dest.COM.new_img, dest.COM.next_undo_LSN );

    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_HEADER_SIZE);
    // cur_off += LOG_HEADER_SIZE;
    // printf("fifth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );

    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_HEADER_SIZE);
    // cur_off += LOG_HEADER_SIZE;
    // printf("sixth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );
    
    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_UPDR_SIZE);
    // cur_off += LOG_UPDR_SIZE;
    // printf("seventh record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s\n", dest.UPD.log_size, dest.UPD.LSN, dest.UPD.prev_LSN, dest.UPD.trx_id, dest.UPD.type, dest.UPD.tid, dest.UPD.page_num, dest.UPD.offset, dest.UPD.data_length, dest.UPD.old_img, dest.UPD.new_img );

    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_HEADER_SIZE);
    // cur_off += LOG_HEADER_SIZE;
    // printf("eighth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );


    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_COMR_SIZE);
    // cur_off += LOG_COMR_SIZE;
    // printf("ninth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d, table_id : %d, pagenum : %d, offset : %d, data_L : %d, old : %s, new : %s, nex_undo : %d\n", dest.COM.log_size, dest.COM.LSN, dest.COM.prev_LSN, dest.COM.trx_id, dest.COM.type, dest.COM.tid, dest.COM.page_num, dest.COM.offset, dest.COM.data_length, dest.COM.old_img, dest.COM.new_img, dest.COM.next_undo_LSN );

    // lseek(log_file, cur_off, SEEK_SET);
    // read(log_file, &dest, LOG_HEADER_SIZE);
    // cur_off += LOG_HEADER_SIZE;
    // printf("tenth record log_size : %d, LSN : %d, prev_LSN : %d, trx_id : %d, type : %d\n", dest.BCR.log_size, dest.BCR.LSN, dest.BCR.prev_LSN, dest.BCR.trx_id, dest.BCR.type );
    
    // log_buff_all_read();
    // exit(1);

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
            uint32_t howMany = tmp_page.I.header.numKeys; //internal?????? leaf??? key??? ????????? node??? ?????? key??? ????????? ???????????????

            if( tmp_page.I.header.isLeaf == 0 ) {  //internal page??? ??????, ?????? page??? queue??? ?????? & page??? key ??????
                enqueue(tmp_page.I.header.special);
                for(int j=0; j<howMany; j++){
                    enqueue(tmp_page.I.records[j].pageN);
                }
                for(int j=0; j<howMany; j++){
                printf("%lld ",tmp_page.I.records[j].key);
                }
            }

            else{  //leaf page??? ?????? key??? ??????
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
            printf("for debug node pagenum_t : %llu   ",tmp);
            page_t tmp_page;
            buff_read_page(table_id, tmp, &tmp_page);
            uint32_t howMany = tmp_page.I.header.numKeys; //internal?????? leaf??? key??? ????????? node??? ?????? key??? ????????? ???????????????

            if( tmp_page.I.header.isLeaf == 0 ) {  //internal page??? ??????, ?????? page??? queue??? ?????? & page??? key ??????
                enqueue(tmp_page.I.header.special);
                for(int j=0; j<howMany; j++){
                    enqueue(tmp_page.I.records[j].pageN);
                }
                for(int j=0; j<howMany; j++){
                printf("%lld ",tmp_page.I.records[j].key);
                }
            }

            else{  //leaf page??? ?????? key??? ??????
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
    buff_read_page(table_id, 0, &header);
    int i = 0;
    pagenum_t tmp_pageN = header.H.rootPnum;
    page_t tmp_page;
    
    if(tmp_pageN == 0){
        return 0;
    }
    
    buff_read_page(table_id, tmp_pageN, &tmp_page);

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
        buff_read_page(table_id, tmp_pageN,&tmp_page);
    }

    return tmp_pageN;
}

int db_find(int table_id, int64_t key, char * ret_val ) {
    if (tableM->tables[table_id-1].dis < 0){
        printf("open_table fail\n");
        return 1;
    }
    int i = 0;
    pagenum_t leafN = find_leaf( table_id, key );
    if(leafN == 0){
        return 1;
    }
    page_t leaf;
    // printf("--------------------------------------\n");
    buff_read_page(table_id, leafN, &leaf);
    
    for( i = 0; i < leaf.L.header.numKeys; i++){
        if(leaf.L.records[i].key == key){
            memcpy(ret_val, leaf.L.records[i].value, VALUE_SIZE);
            return 0;
        }
    }
    
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

    //????????? ?????? 0?????? ?????????
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
        n.I.records[i+1].pageN = n.I.records[i].pageN; //pageNum ????????????
        n.I.records[i+1].key = n.I.records[i].key; //key ??????
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
    old_page.I.records[split-1].key = 0; //???????????? ??????????????? ??????????????????
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
    if(db_find(table_id, key,ret_val) == 0 ){
        return 1;
    }

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
    for( i = 0; i <parent.I.header.numKeys;i++){ //ex) 0?????? ???????????? ?????? ???????????? ????????? ?????? 1???????????? index 0??? return ????????? ??????
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
    } //nN??? 1 neighborN??? 0

    page_t n, neighbor;
    buff_read_page(table_id, nN, &n);
    buff_read_page(table_id, neighborN, &neighbor);

    pagenum_t parentN = n.I.header.parent;

    neighbor_insertion_index = neighbor.I.header.numKeys; //internal?????? leaf??? header??? ?????????

    if (n.I.header.isLeaf == 0) { //internal ??????
        neighbor.I.records[neighbor_insertion_index].key = k_prime; //???????????? ????????? ???
        neighbor.I.records[neighbor_insertion_index].pageN = n.I.header.special; //neighbor??? ?????? ?????? ??????
        neighbor.I.header.numKeys += 1;
        n_end = n.I.header.numKeys;
        for( i = neighbor_insertion_index + 1, j=0; j < n_end; i++, j++) {
            neighbor.I.records[i].key = n.I.records[j].key;
            neighbor.I.records[i].pageN = n.I.records[j].pageN;
            neighbor.I.header.numKeys += 1;
            n.I.header.numKeys -= 1;
        } //???????????? ?????? n??? ????????? neighbor??? ??????
        
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
        n.I.records[0].pageN = n.I.header.special;  //n??? special ?????? ????????????
        n.I.header.special = neighbor.I.records[neighbor.I.header.numKeys-1].pageN; //special??? neighbor??? ????????? pageN ????????????
        tmpN = neighbor.I.records[neighbor.I.header.numKeys-1].pageN; //tmp??? neighbor??? ????????? ?????? ???????????? parent??? n?????? ??????
        buff_read_page(table_id,tmpN,&tmp);
        tmp.I.header.parent = nN;
        neighbor.I.records[neighbor.I.header.numKeys-1].pageN = 0; //neighbor??? ????????? parent ?????????
        n.I.records[0].key = k_prime; //n??? ????????? ?????? ????????????
        parentN = n.I.header.parent; //parnet page ???????????? parnet ????????? ???????????????
        buff_read_page(table_id,parentN, &parent);
        parent.I.records[k_prime_index].key = neighbor.I.records[neighbor.I.header.numKeys-1].key;

    }

    else {  
        n.I.records[n.I.header.numKeys].key = k_prime;  //neighbor?????? ????????? key?????? ?????? ??????
        n.I.records[n.I.header.numKeys].pageN = neighbor.I.header.special;
        tmpN = neighbor.I.header.special; //tmp??? neighbor??? ????????? ?????? ???????????? parent??? n?????? ??????
        buff_read_page(table_id,tmpN,&tmp);
        tmp.I.header.parent = nN;
        parentN = n.I.header.parent; //parnet page ???????????? parnet ????????? ???????????????
        buff_read_page(table_id,parentN, &parent);
        parent.I.records[k_prime_index].key = neighbor.I.records[0].key;

        neighbor.I.header.special = neighbor.I.records[0].pageN;  //neighbor??? key??? ????????? ????????? ????????????
        for( i = 0; i<neighbor.I.header.numKeys-1 ; i++){
            neighbor.I.records[i].key = neighbor.I.records[i+1].key;
            neighbor.I.records[i].pageN = neighbor.I.records[i+1].pageN;
        }
        neighbor.I.records[i].key = 0; //???????????? ???????????? ??? 0?????? ?????????
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
    buff_read_page(table_id,nN,&n); //remove_entry_from_node??? ????????? ???????????? ?????? ???????????????

    buff_read_page(table_id, 0, &header);
    if (nN == header.H.rootPnum) 
        return adjust_root(table_id, header.H.rootPnum);

    if(n.I.header.numKeys != 0){
        return nN;
    }

    neighbor_index = get_neighbor_index( table_id, nN );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
    k_prime = parent.I.records[k_prime_index].key; //parent??? ????????? internal node??????
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
    
    if ( db_find(table_id, key,ret_val) == 0 && key_leaf != 0) {
        rootN = delete_entry(table_id, key_leaf, key);
        free(ret_val);
        return 0;
    }
    free(ret_val);
    return 1;
}

