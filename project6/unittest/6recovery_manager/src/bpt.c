#include "bpt.h"

page_t header;
tManager * tableM;
buffer * buff;
log_buffer * log_buff;
int log_file;
recoveryManager * recovM;

int open_table(char* pathname){
    int ret = open_table_sec(pathname);

    return ret;
}

int close_table(int table_id){
    int ret = close_table_sec(table_id);
    
    return ret;
}


int init_db(int num_buf, char * log_path){
    int ret = init_buff(num_buf);
    ret = init_log_buff(num_buf, log_path);
    printf("Init log_buff succ\n");
    printf("After init_buf, global lsn : %d, prev lsn : %d\n",log_buff->global_LSN, log_buff->global_prev_LSN);
    init_recoveryM();
    printf("Init recoveryM succ\n");

    recov_analysis(log_path, "msg.path");
    recov_redo(log_path, "msg.path");
    recov_undo(log_path, "msg.path");

    log_buff_flush();


    log_buff_read_all_file();
    exit(1);

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
                printf("page_lsn : %lld  ",tmp_page.L.header.page_LSN);
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
    
    if ( db_find(table_id, key,ret_val) == 0 && key_leaf != 0) {
        rootN = delete_entry(table_id, key_leaf, key);
        free(ret_val);
        return 0;
    }
    free(ret_val);
    return 1;
}




//start recovery
void init_recoveryM(void){
    recovM = (recoveryManager *)malloc(sizeof(recoveryManager));
    recovM->head = NULL;
    recovM->log_cnt = 0;
    recovM->redo_head = NULL;
    recovM->undo_head = NULL;
    recovM->loser_cnt = 0;
    recovM->loser_entry = NULL;
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


void recov_analysis(char * log_path, char * logmsg_path){
    int ret = 0;
    off_t offset = 0;

    log_record_t tmp_record;
    tmp_record.BCR.type = 0;

    r_trx * tmp_ptr = NULL;


//analysis winner & loser
    while(ret != -1){
        ret = log_buff_search_record(log_file, offset, &tmp_record);
        if(ret == -2){
            printf("log_file is empty\n");
            return ;
        }
        // printf("after search_record, LSN : %d, trx_id : %d\n",tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
        if(tmp_record.BCR.type == LOG_BEG_TYPE ){
            tmp_ptr = r_trx_begin(tmp_record.BCR.trx_id);
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
    // print_recovM();
    // recovM->loser_entry = who_is_loser();
    // for(int j = 0; j < recovM->loser_cnt; j++){
    //     printf("%dst loser trx : %d\n",j+1,recovM->loser_entry[j]);
    // }
}


void recov_redo(char * log_path, char * logmsg_path){
    int ret = 0;
    off_t offset = 0;

    log_record_t tmp_record;
    tmp_record.BCR.type = 0;

    r_trx * tmp_ptr = NULL;

    printf("\nREDO START\n");

// //make redo_entry
    while(ret != -1){
        ret = log_buff_search_record(log_file, offset, &tmp_record);
        if(ret == -2){
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
        return;
    }
    while(tmp_ptr2 != NULL){
        memcpy(&tmp_record, &tmp_ptr2->content, sizeof(log_record_t));
        if( tmp_record.BCR.type == LOG_BEG_TYPE ){
            tmp_ptr2 = tmp_ptr2->next;
            printf("LSN : %d, BEGIN, trx_id : %d\n", tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
            continue;
        }
        if( tmp_record.BCR.type == LOG_COMM_TYPE ){
            printf("LSN : %d, COMMIT, trx_id : %d\n", tmp_record.BCR.LSN, tmp_record.BCR.trx_id);
            tmp_ptr2 = tmp_ptr2->next;
            continue;
        }
        if( tmp_record.BCR.type == LOG_ROLL_TYPE){
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
        if(tmp_page.L.header.page_LSN >= tmp_record.UPD.LSN){
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
            printf("LSN : %d, UPDATE, trx_id : %d\n", tmp_record.UPD.LSN, tmp_record.UPD.trx_id);
        }

        if(tmp_record.UPD.type == LOG_COM_TYPE){
            printf("LSN : %d, COMPENSATE, trx_id : %d\n", tmp_record.UPD.LSN, tmp_record.UPD.trx_id);  
        }
        tmp_ptr2 = tmp_ptr2->next;
        // exit(1);
    }
    printf("\nREDO END\n");
    printf("\n");

}

//다다시
void recov_undo(char * log_path, char * logmsg_path){
    int ret = 0;
    off_t offset = 0;
    log_record_t tmp_record;

    printf("\nUNDO START\n");

    recovM->loser_entry = who_is_loser();
    

    for(int i = 0; i < recovM->loser_cnt; i++){
        r_trx * tmp_rt = r_trx_find(recovM->loser_entry[i]);
        // printf("last_LSN : %d\n",tmp_rt->last_LSN);
        ret = log_buff_search_record(log_file, tmp_rt->last_LSN, &tmp_record);
        add_undo_entry(&tmp_record);
    }
    
    print_recovM();

    un_entry * tmp_entry = del_undo_entry();
    

    while(tmp_entry != NULL){
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
                // print_log_buff();
                buff_write_page_no_lock(tmp_entry->content.UPD.tid, tmp_entry->content.UPD.page_num, &tmp_page);
                print_log_buff();
            }

        }

        log_record_t tmp_prev_LSN_record;
        memset(&tmp_prev_LSN_record, 0, LOG_COMR_SIZE);
        ret = log_buff_search_record(log_file, tmp_prev_LSN, &tmp_prev_LSN_record);

        if(tmp_entry->content.COM.type == LOG_UPD_TYPE && tmp_prev_LSN_record.COM.type == LOG_UPD_TYPE ){
            ret = log_buff_search_record(log_file, tmp_prev_LSN, &tmp_record);
            add_undo_entry(&tmp_record);
        }
        if(tmp_entry->content.COM.type == LOG_COM_TYPE && tmp_entry->content.COM.next_undo_LSN != -1){
            ret = log_buff_search_record(log_file, tmp_LSN, &tmp_record);
            add_undo_entry(&tmp_record);
        }
        else{
            log_buff_write(tar_rtrx->last_LSN, tar_rtrx->id, LOG_ROLL_TYPE, 0, 0, 0, "\0", "\0");
        }
        // print_recovM();
        tmp_entry = del_undo_entry();
        print_recovM();
    }
    
    printf("\nUNDO END\n");
}