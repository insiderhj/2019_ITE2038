#include "bpt.h"

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    open_table("database");

    
    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
            case 'd':
                scanf("%ld", &key);
                db_delete(key);
                break;
            case 'i':
                scanf("%ld %s", &key, value);
                db_insert(key, value);
                break;
            case 'c':
                for (int i = 1; i <= 33; i++) {
                    int num = i;
                    for (int j = 0; j < 4; j++) {
                        value[j] = '0' + num % 10;
                        num /= 10;
                    }
                    db_insert(i, value);
                    printf("%d inserted\n", i);
                }   
                break;
            case 'f':
                scanf("%ld", &key);
                if (db_find(key, value) == NOT_FOUND) printf("key %d not found\n", key);
                else printf("%d: %s\n", key, value);
                break;
            case 'p':
                print_tree();
                break;
            case 'q':
                while (getchar() != (int)'\n');
                return 0;
                break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return 0;
}
