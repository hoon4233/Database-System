#include <string.h> //debug
#include "file.h"
#include "bpt.h"

// MAIN
extern int file;
extern page_t header;
extern int tableNum;

int main( int argc, char ** argv ) {


    int64_t key;
    char value[VALUE_SIZE];
    char instruction;
    int ret_int;
    if(argc >= 2){
        tableNum = open_table(argv[1]);
    }
    else{
        tableNum = open_table("sample.db");
    }

    if(file<0){
        printf("open_table fail\n");
        return -1;
    }

    // leaf page split test, i 31 31 하면 됨
    // for( int j = 0; j < 31; j++){
    //     key = j;
    //     // char tmp_v[120];
    //     char * tmp_v = (char*) malloc(120);
    //     sprintf(tmp_v,"%d",j);
    //     memcpy(value, tmp_v,120);
    //     int ret_int2 = db_insert(key,value);
    //     free(tmp_v);
    // }
    // print_tree();

    // internal page split test, i 4000 4000 하면 됨
    // for( int j = 0; j < 4000; j++){
    //     key = j;
    //     // char tmp_v[120];
    //     char * tmp_v = (char*) malloc(120);
    //     sprintf(tmp_v,"%d",j);
    //     memcpy(value, tmp_v,120);
    //     int ret_int2 = db_insert(key,value);
    //     free(tmp_v);
    // }
    // print_tree();

    // delete leaf page test, d 15 하면 됨
    // for( int j = 0; j < 32; j++){
    //     key = j;
    //     // char tmp_v[120];
    //     char * tmp_v = (char*) malloc(120);
    //     sprintf(tmp_v,"%d",j);
    //     memcpy(value, tmp_v,120);
    //     int ret_int2 = db_insert(key,value);
    //     free(tmp_v);
    // }
    // for( int j = 0; j < 15; j++){
    //     key = j;
    //     int ret_int2 = db_delete(key);
    // }
    // print_tree();

    //merge test, d 1967, d1983 하면 됨
    // for( int j = 0; j < 5000; j++){
    //     key = j;
    //     // char tmp_v[120];
    //     char * tmp_v = (char*) malloc(120);
    //     sprintf(tmp_v,"%d",j);
    //     memcpy(value, tmp_v,120);
    //     int ret_int2 = db_insert(key,value);
    //     free(tmp_v);
    // }
    // for( int j = 0; j < 1967; j++){
    //     key = j;
    //     int ret_int2 = db_delete(key);
    // }
    // for( int j = 1968; j < 1983; j++){
    //     key = j;
    //     int ret_int2 = db_delete(key);
    // }
    // print_tree();

    //redistribution test, d 1967, d1983 하면 됨
    // for( int j = 0; j < 5990; j++){
    //     key = j;
    //     // char tmp_v[120];
    //     char * tmp_v = (char*) malloc(120);
    //     sprintf(tmp_v,"%d",j);
    //     memcpy(value, tmp_v,120);
    //     int ret_int2 = db_insert(key,value);
    //     free(tmp_v);
    // }
    // // for( int j = 0; j > -100; j--){
    // //     key = j;
    // //     char tmp_v[120];
    // //     sprintf(tmp_v,"%d",j);
    // //     memcpy(value, tmp_v,120);
    // //     int ret_int2 = db_insert(key,value);
    // // }
    // for( int j = 0; j < 1967; j++){
    //     key = j;
    //     int ret_int2 = db_delete(key);
    // }
    // for( int j = 1968; j < 1983; j++){
    //     key = j;
    //     int ret_int2 = db_delete(key);
    // }
    // // // print_tree2();
    // print_tree();
    

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%lldd", &key);
            ret_int = db_delete(key);
            print_tree();
            break;
        case 'i': 
            scanf("%lld %s", &key, value);
            ret_int = db_insert(key, value);
            print_tree();
            break;
        case 'f':
            scanf("%lld", &key);
            ret_int = db_find(key, value);
            printf("find result : %s\n", value);
            break;
        
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;

        case 't':
            print_tree();
            break;
            
        default:
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    close(file);
    return EXIT_SUCCESS;
}
