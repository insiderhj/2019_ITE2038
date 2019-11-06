#include "bpt.h"

int fds[10];

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    int file_num = 0;
    int first_file, second_file;
    char file_name[512];
    init_db(500);

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
            case 'o':
                scanf("%s", file_name);
                file_num = open_table(file_name);
                printf("%s: %d\n", file_name, file_num);
                break;
            case 'd':
                scanf("%d %ld", &file_num, &key);
                db_delete(file_num, key);
                break;
            case 'i':
                scanf("%d %ld %s", &file_num, &key, value);
                db_insert(file_num, key, value);
                break;
	        case 'f':
                scanf("%d %ld", &file_num, &key);
                if (db_find(file_num, key, value) == NOT_FOUND) printf("key %ld not found\n", key);
                else printf("%ld: %s\n", key, value);
                break;
            case 'j':
                scanf("%d %d %s", &first_file, &second_file, file_name);
                join_table(fds[first_file], fds[second_file], file_name);
                break;
            case 'l':
                for (int i = 0; i < 10; i++) {
                    if (fds[i]) printf("%d. %s\n", fds[i], pathnames[i]);
                }
                break;
            case 'p':
                scanf("%d", &file_num);
                print_tree(file_num);
                break;
            case 'q':
                while (getchar() != (int)'\n');
                shutdown_db();
                return 0;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return 0;
}
