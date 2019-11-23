#include "bpt.hpp"
#include <time.h>

int test(char* pathname);

int main( int argc, char ** argv ) {
    char instruction;
    int64_t key;
    char value[VALUE_SIZE];
    int file_num = 0;
    int first_file, second_file;
    char file_name[512];
    int result;
    init_db(100000);

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
            case 'o':
                scanf("%s", file_name);
                result = file_num = open_table(file_name);
                if (result == 0) printf("%d. %s\n", file_num, file_name);
                break;
            case 'c':
                scanf("%d", &file_num);
                result = close_table(file_num);
                break;
            case 'd':
                scanf("%d %ld", &file_num, &key);
                result = db_delete(file_num, key);
                break;
            case 'i':
                scanf("%d %ld %s", &file_num, &key, value);
                result = db_insert(file_num, key, value);
                break;
	        case 'f':
                scanf("%d %ld", &file_num, &key);
                result = db_find(file_num, key, value);
                if (result == 0) printf("%ld: %s\n", key, value);
                break;
            case 'j':
                scanf("%d %d %s", &first_file, &second_file, file_name);
                result = join_table(first_file, second_file, file_name);
                break;
            case 'l':
                for (int i = 0; i < 10; i++) {
                    if (fds[i]) printf("%d. %s\n", fds[i], pathnames[i]);
                }
                break;
            case 'p':
                scanf("%d %s", &file_num, file_name);
                result = print_table(file_num, file_name);
                break;
            case 't':
                result = test("input.txt");
                break;
            case 'q':
                while (getchar() != (int)'\n');
                shutdown_db();
                return 0;
        }
        if (result < 0) printf("ERROR CODE: %d\n", result);
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");
    return 0;
}

int test(char* pathname) {
    FILE* fp = fopen(pathname, "r");
    int instruction, count = 0, table_size = 0;
    int tables[10];
    char value[VALUE_SIZE];
    int table_num;
    int result;
    int64_t key;

    double start_time = (double)clock() / CLOCKS_PER_SEC;
    while (fscanf(fp, "%d", &instruction) != EOF) {
        count++;
        if (count % 10000 == 0) printf("%d\n", count);
        switch (instruction) {
            case 0:
                fscanf(fp, "%s", value);
                tables[table_size++] = open_table(value);
                break;
            case 1:
                fscanf(fp, "%d %ld %s", &table_num, &key, value);
                result = db_insert(tables[table_num - 1], key, value);
                break;
            case 2:
                fscanf(fp, "%d %ld", &table_num, &key);
                result = db_find(tables[table_num - 1], key, NULL);
                break;
            case 3:
                fscanf(fp, "%d %ld", &table_num, &key);
                result = db_delete(tables[table_num - 1], key);
                break;
            case 4:
                fscanf(fp, "%d", &table_num);
                result = close_table(tables[table_num - 1]);
                break;
            case 10:
                shutdown_db();
                double end_time = (double)clock() / CLOCKS_PER_SEC;
                printf(" / %lf sec elapsed\n", end_time - start_time);
                break;
        }
    }
    fclose(fp);

    return 0;
}
