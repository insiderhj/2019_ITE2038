#include "bpt.h"

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    open_table("database_insiderhj");

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
            case 'f':
                scanf("%ld", &key);
                if (db_find(key, value) == NOT_FOUND) printf("key %ld not found\n", key);
                else printf("%ld: %s\n", key, value);
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
