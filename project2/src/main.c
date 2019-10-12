#include "bpt.h"

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    open_table("database");

    // for test
    int keys[30] = {19, 18, 20, 17, 4, 12, 3, 14, 28, 2, 1, 27, 22, 24, 25, 11, 15, 16, 23, 6, 26, 13, 21, 5, 30, 10, 29, 8, 9, 7};

    value[0] = 'a';
    value[1] = '\0';
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
                for (int i = 0; i < 30; i++) {
                    int num = keys[i];
                    db_insert(num, value);
                    printf("%d inserted\n", keys[i]);
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
