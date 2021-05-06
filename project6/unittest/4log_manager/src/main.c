#include "bpt.h"

extern buffer * buff;
extern tManager * tableM;
extern page_t header;

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    int table_id;

    int ret_int;
    int ret;
    int tableNum;
    
    if(argc >= 2){
        ret = init_db(100, "logfile.data");
        tableNum = open_table(argv[1]);
    }
    else{
        ret = init_db(1000, "logfile.data");
        tableNum = open_table("sample.db");
    } 
    

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%d %lldd", &table_id, &key);
            ret_int = db_delete(table_id,key);
            print_tree(table_id);
            break;
        case 'i': 
            scanf("%d %lld %s", &table_id, &key, value);
            ret_int = db_insert(table_id,key, value);
            print_tree(table_id);
            break;
        case 'f':
            scanf("%d %lld", &table_id, &key);
            ret_int = db_find(table_id,key, value);
            printf("find result : %s\n", value);
            break;
        
        case 'q':
            while (getchar() != (int)'\n');
            for(int i=1; i<10; i++){
                close_table(i);
            }
            shutdown_db();
            return EXIT_SUCCESS;
            break;

        case 't':
            scanf("%d", &table_id);
            print_tree(table_id);
            break;
            
        default:
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return EXIT_SUCCESS;
}
