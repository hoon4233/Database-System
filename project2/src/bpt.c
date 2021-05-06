#include "file.h"
#include "bpt.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

bool verbose_output = false;

int file;
page_t header;
int tableNum = 0;


int open_table(char* pathname){
    int tmp_tableNum = tableNum;
    tableNum += 1;

    if( file_open_talbe(pathname) == -1 )
        return -1;

    return tmp_tableNum;
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


void print_tree() { //print
 
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
            page_t tmp_page;
            file_read_page(tmp, &tmp_page);
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

void print_tree2() {

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
            file_read_page(tmp, &tmp_page);
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

pagenum_t find_leaf( int64_t key ) {
    int i = 0;
    pagenum_t tmp_pageN = header.H.rootPnum;
    page_t tmp_page;
    
    if(tmp_pageN == 0){
        return 0;
    }
    
    file_read_page(tmp_pageN, &tmp_page);

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
        file_read_page(tmp_pageN,&tmp_page);
    }

    return tmp_pageN;
}

int db_find( int64_t key, char * ret_val ) {
    if (file < 0){
        printf("open_table fail\n");
        return 1;
    }
    int i = 0;
    pagenum_t leafN = find_leaf( key );
    if(leafN == 0){
        return 1;
    }
    page_t leaf;
    file_read_page(leafN, &leaf);
    
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
pagenum_t make_internal( void ) {
    pagenum_t tmp_pageN = file_alloc_page();
    page_t tmp_page;
    file_read_page(tmp_pageN,&tmp_page);

    tmp_page.I.header.parent = 0;
    tmp_page.I.header.isLeaf = 0;
    tmp_page.I.header.numKeys = 0;
    tmp_page.I.header.special = 0;
    file_write_page(tmp_pageN, &tmp_page);

    return tmp_pageN;
}

pagenum_t make_leaf( void ) {
    pagenum_t tmp_pageN = file_alloc_page();
    page_t tmp_page;
    file_read_page(tmp_pageN,&tmp_page);

    tmp_page.L.header.parent = 0;
    tmp_page.L.header.isLeaf = 1;
    tmp_page.L.header.numKeys = 0;
    tmp_page.L.header.special = 0;
    file_write_page(tmp_pageN, &tmp_page);

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


int get_left_index(pagenum_t parentN, pagenum_t leftN) {

    int left_index = 0;
    page_t parent, left;
    file_read_page(parentN, &parent);
    file_read_page(leftN, &left);
    if (parent.I.header.special == leftN) return -1;
    while (left_index <= parent.I.header.numKeys-1 && 
            parent.I.records[left_index].pageN != leftN)
        left_index++;
    
    return left_index;
}

pagenum_t insert_into_leaf( pagenum_t leaf_pageN, int64_t key, char * value ) {

    int i, insertion_point;
    page_t leaf_page;
    file_read_page(leaf_pageN,&leaf_page);

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
    
    
    file_write_page(leaf_pageN,&leaf_page);

    return leaf_pageN;
}

pagenum_t insert_into_leaf_after_splitting( pagenum_t leaf_pageN, int64_t key, char * value) {
    page_t leaf;
    file_read_page(leaf_pageN, &leaf);

    pagenum_t new_leafN = make_leaf();
    page_t new_leaf;
    file_read_page(new_leafN, &new_leaf);

    // pagenum_t tmp_leafN = make_leaf();
    // page_t tmp_leaf; //나중에 file_free_page 해줘야함
    // file_read_page(tmp_leafN, &tmp_leaf);
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

    file_write_page(leaf_pageN,&leaf);
    file_write_page(new_leafN,&new_leaf);

    return insert_into_parent(leaf_pageN, new_key, new_leafN);
}

pagenum_t insert_into_node( pagenum_t nN, 
        int left_index, int64_t key, pagenum_t rightN) {
    int i;
    page_t n;
    file_read_page(nN, &n);    

    for (i = n.I.header.numKeys-1; i > left_index; i--){
        n.I.records[i+1].pageN = n.I.records[i].pageN; //pageNum 밀어주기
        n.I.records[i+1].key = n.I.records[i].key; //key 밀기
    }

    n.I.records[left_index+1].pageN = rightN;
    n.I.records[left_index+1].key = key;
    n.I.header.numKeys += 1;
    file_write_page(nN, &n);
    
    return 0;
}

pagenum_t insert_into_node_after_splitting( pagenum_t old_pageN, int left_index, 
        int64_t key, pagenum_t rightN) {
    page_t old_page;
    file_read_page(old_pageN, &old_page);

    pagenum_t new_pageN = make_internal();
    page_t new_page;
    file_read_page(new_pageN, &new_page);

    // pagenum_t tmp_pageN = file_alloc_page();  //나중에 file_free_page 해줘야함
    // page_t tmp_page;
    // file_read_page(tmp_pageN, &tmp_page);
    tmppage_t * tmp_page = (tmppage_t*) malloc(sizeof(tmppage_t));
    make_tmpinternal(tmp_page);
    // printf("In into_node_after_splitting sizeof(tmp_page : %d\n",sizeof(tmp_page));

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

    // file_free_page(tmp_pageN);
    free(tmp_page);

    pagenum_t childN = new_page.I.header.special;
    page_t child;
    file_read_page(childN,&child);
    child.I.header.parent = new_pageN;
    file_write_page(childN,&child);

    for ( i = 0; i< new_page.I.header.numKeys; i++){
        childN = new_page.I.records[i].pageN;
        file_read_page(childN,&child);
        child.I.header.parent = new_pageN;
        file_write_page(childN,&child);
    }

    file_write_page(old_pageN,&old_page);
    file_write_page(new_pageN,&new_page);

    return insert_into_parent( old_pageN, k_prime, new_pageN);
}

pagenum_t insert_into_parent( pagenum_t leftN, int64_t key, pagenum_t rightN) {

    int left_index;

    page_t leftP, rightP, parentP;
    file_read_page(leftN, &leftP);
    file_read_page(rightN, &rightP);

    pagenum_t parentN = leftP.I.header.parent;
    file_read_page(parentN, &parentP);

    if ( parentN == 0 ){
        return insert_into_new_root(leftN, key, rightN);
    }


    left_index = get_left_index(parentN, leftN);

    if ( parentP.I.header.numKeys < INTERNAL_ORDER-1 ){
        return insert_into_node(parentN, left_index, key, rightN);
    }

    return insert_into_node_after_splitting( parentN, left_index, key, rightN );
}

pagenum_t insert_into_new_root(pagenum_t leftN, int64_t key, pagenum_t rightN) {

    pagenum_t rootN = make_internal();
    page_t root, left, right;
    file_read_page(rootN, &root);
    root.I.records[0].key = key;
    root.I.header.special = leftN;
    root.I.records[0].pageN = rightN;
    root.I.header.numKeys += 1;
    root.I.header.parent = 0;
    
    file_read_page(leftN, &left);
    file_read_page(rightN, &right);
    left.I.header.parent = rootN;
    right.I.header.parent = rootN;

    file_write_page(rootN, &root);
    file_write_page(leftN, &left);
    file_write_page(rightN, &right);

    header.H.rootPnum = rootN;
    file_write_page(0,&header);

    return rootN;
}

void start_new_tree(int64_t key, char * value) {

    pagenum_t rootN = make_leaf();
    page_t root;
    file_read_page(rootN,&root);
    
    root.L.header.parent = 0;
    root.L.header.numKeys += 1;
    root.L.records[0].key = key;
    memcpy(root.L.records[0].value, value, VALUE_SIZE);
    file_write_page(rootN,&root);

    header.H.rootPnum = rootN;
    file_write_page(0,&header);
    
    file_read_page(rootN,&root);
}


int db_insert(int64_t key, char* value){
    if (file < 0){
        printf("open_table fail\n");
        return 1;
    }
    char * ret_val = (char*)malloc(sizeof(char)*120);
    pagenum_t rootN;
    if(db_find(key,ret_val) == 0 ){
        return 1;
    }

    if(header.H.rootPnum == 0){
        start_new_tree(key, value);
        return 0;
    }

    pagenum_t leaf_pageN = find_leaf(key);
    page_t leaf_page;
    file_read_page(leaf_pageN,&leaf_page);

    if(leaf_page.L.header.numKeys < LEAF_ORDER-1){
        rootN = insert_into_leaf(leaf_pageN, key, value);
        return 0;
    }

    rootN = insert_into_leaf_after_splitting(leaf_pageN, key, value);
    return 0;

}


// DELETION
int get_neighbor_index( pagenum_t nN ) {
    int i;
    pagenum_t parentN;
    page_t n, parent;
    file_read_page(nN,&n);
    parentN = n.I.header.parent;
    file_read_page(parentN, &parent);

    if(parent.I.header.special == nN) return -1;
    for( i = 0; i <parent.I.header.numKeys;i++){ //ex) 0번째 포인터가 나를 가리키고 있으면 나는 1번째이고 index 0이 return 되는게 옳다
        if(parent.I.records[i].pageN == nN)
            return i;
    }
    printf("Search for nonexistent pointer to node in parent.\n");
    exit(EXIT_FAILURE);

}


pagenum_t remove_entry_from_node( pagenum_t nN, int64_t key ) {

    int i = 0;
    page_t n;
    file_read_page(nN,&n);
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

    file_write_page(nN,&n);
    return nN;
}


pagenum_t adjust_root(pagenum_t rootN) {

    page_t root;
    file_read_page(rootN, &root);
    pagenum_t new_rootN;
    page_t new_root;
    int flag = root.I.header.isLeaf ? 1 : 0;

    if(root.I.header.numKeys > 0 ){
        return rootN;
    }

    if(flag == 0){
        header.H.rootPnum = root.I.header.special;
        
        new_rootN = root.I.header.special;
        file_read_page(new_rootN, &new_root);
        new_root.I.header.parent = 0;

        file_write_page(0,&header);
        file_write_page(new_rootN,&new_root);
    }
    else{
        header.H.rootPnum = 0;
        new_rootN = 0;

        file_write_page(0,&header);
    }
    file_free_page(rootN);

    return new_rootN;

}

pagenum_t coalesce_nodes( pagenum_t nN, pagenum_t neighborN, int neighbor_index, int64_t k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    pagenum_t tmpN, rootN;

    if (neighbor_index == -1) {
        tmpN = nN;
        nN = neighborN;
        neighborN = tmpN;
    } //nN이 1 neighborN이 0

    page_t n, neighbor;
    file_read_page(nN, &n);
    file_read_page(neighborN, &neighbor);

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
        file_read_page(childN, &child);
        child.I.header.parent = neighborN;
        file_write_page(childN,&child);
        for( i = 0; i < neighbor.I.header.numKeys; i++){
            childN = neighbor.I.records[i].pageN;
            file_read_page(childN, &child);
            child.I.header.parent = neighborN;
            file_write_page(childN, &child);
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
    

    file_write_page(neighborN,&neighbor);
    file_write_page(nN,&n);
    rootN = delete_entry(parentN, k_prime);
    file_free_page(nN);
    
    return rootN;
}


pagenum_t redistribute_nodes( pagenum_t nN, pagenum_t neighborN, int neighbor_index, 
        int k_prime_index, int64_t k_prime) {  

    int i;
    pagenum_t tmpN, parentN;
    page_t n, neighbor, tmp, parent;
    file_read_page(nN, &n);
    file_read_page(neighborN, &neighbor);
    

    if (neighbor_index != -1) {
        n.I.records[0].pageN = n.I.header.special;  //n의 special 한칸 밀어주고
        n.I.header.special = neighbor.I.records[neighbor.I.header.numKeys-1].pageN; //special에 neighbor의 마지막 pageN 읽어오고
        tmpN = neighbor.I.records[neighbor.I.header.numKeys-1].pageN; //tmp에 neighbor의 마지막 자식 불러와서 parent를 n으로 설정
        file_read_page(tmpN,&tmp);
        tmp.I.header.parent = nN;
        neighbor.I.records[neighbor.I.header.numKeys-1].pageN = 0; //neighbor의 마지막 parent 초기화
        n.I.records[0].key = k_prime; //n에 새로운 키값 넣어주고
        parentN = n.I.header.parent; //parnet page 불러와서 parnet 페이지 조정해주기
        file_read_page(parentN, &parent);
        parent.I.records[k_prime_index].key = neighbor.I.records[neighbor.I.header.numKeys-1].key;

    }

    else {  
        n.I.records[n.I.header.numKeys].key = k_prime;  //neighbor에서 가져온 key값과 자식 설정
        n.I.records[n.I.header.numKeys].pageN = neighbor.I.header.special;
        tmpN = neighbor.I.header.special; //tmp에 neighbor의 첫번째 자식 불러와서 parent를 n으로 설정
        file_read_page(tmpN,&tmp);
        tmp.I.header.parent = nN;
        parentN = n.I.header.parent; //parnet page 불러와서 parnet 페이지 조정해주기
        file_read_page(parentN, &parent);
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

    file_write_page(nN,&n);
    file_write_page(tmpN,&tmp);
    file_write_page(parentN,&parent);
    file_write_page(neighborN,&neighbor);

    return nN;
}

pagenum_t delete_entry( pagenum_t nN, int64_t key ) {
    int min_keys;
    int neighbor_index;
    int k_prime_index;
    int64_t k_prime;
    page_t n;
    file_read_page(nN,&n);
    pagenum_t parentN = n.I.header.parent;
    page_t parent;
    file_read_page(parentN,&parent);
    pagenum_t neighborN;

    nN = remove_entry_from_node(nN,key);
    file_read_page(nN,&n); //remove_entry_from_node의 변경된 데이터를 다시 읽어와야함

    if (nN == header.H.rootPnum) 
        return adjust_root(header.H.rootPnum);

    if(n.I.header.numKeys != 0){
        return nN;
    }

    neighbor_index = get_neighbor_index( nN );
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
    file_read_page(neighborN, &neighbor);
    int capacity = n.I.header.isLeaf ? LEAF_ORDER : INTERNAL_ORDER;

    if ( neighbor.I.header.numKeys+1 == capacity && n.I.header.isLeaf == 0 ){
        return redistribute_nodes(nN, neighborN, neighbor_index, k_prime_index, k_prime);
    }
    else{
        return coalesce_nodes(nN, neighborN, neighbor_index, k_prime);
    }

}



/* Master deletion function.
 */
int db_delete( int64_t key ) {
    if (file < 0){
        printf("open_table fail\n");
        return 1;
    }
    pagenum_t rootN;
    char * ret_val = (char*)malloc(sizeof(char)*120);
    pagenum_t key_leaf = find_leaf(key);
    
    if ( db_find(key,ret_val) == 0 && key_leaf != 0) {
        rootN = delete_entry(key_leaf, key);
        free(ret_val);
        return 0;
    }
    free(ret_val);
    return 1;
}

