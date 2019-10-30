#include "bpt.h"

int fds[10];

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    int file_num = 0;
    init_db(500);
    open_table("database_insiderhj");

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
            case 'd':
                scanf("%ld", &key);
                db_delete(fds[0], key);
                break;
            case 'i':
                scanf("%ld %s", &key, value);
                db_insert(fds[0], key, value);
                break;
	        case 'f':
                scanf("%ld", &key);
                if (db_find(fds[0], key, value) == NOT_FOUND) printf("key %ld not found\n", key);
                else printf("%ld: %s\n", key, value);
                break;
            case 'q':
                while (getchar() != (int)'\n');
                shutdown_db();
                return 0;
        }
        print_tree(fds[0]);
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return 0;
}
