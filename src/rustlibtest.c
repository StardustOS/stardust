//
// Created by Josh on 25/05/2021.
//

#ifdef RUST_LIB_TEST
#include "stardust_db.h"
#include <pthread.h>
#include <stdio.h>
#include <os/blkfront.h>
#include <fs/ff.h>
#include <fs/osal.h>
#include <fdlibm/fdlibm.h>

double round(double x) {
    if (x >= 0.0) {
        double y = floor(x);
        if (x - y >= 0.5) {
            y += 1.0;
        }
        return y;
    }
    else {
        double y = ceil(x);
        if (y - x >= 0.5) {
            y -= 1.0;
        }
        return y;
    }
}

static void* main_thread(void* a) {
    FATFS FatFs;
    Db db;
    RowSet row_set = ROW_SET_INIT;
    int result;
    char error[100];
    error[0] = '\0';
    if (init_disk()) {
        while (!blk_has_initialised()) {}
        if (f_mount(&FatFs, "", 1) == FR_OK) {
            printf("Starting database\n");
            result = temp_db(&db);
            if (result != STARDUST_DB_OK) goto error;

            result = execute_query(&db, "CREATE TABLE test (name string, age int);"
                                        "INSERT INTO test VALUES ('Josh', 23), ('Jim', 25)", &row_set, error, 100);
            if (result != STARDUST_DB_OK) goto error;

            result = execute_query(&db, "SELECT * FROM test", &row_set, error, 100);
            if (result != STARDUST_DB_OK) goto error;

            int ended;
            result = is_end(&row_set, &ended);
            if (result != STARDUST_DB_OK) goto error;

            while (!ended) {
                char name[10];
                IntegerStorage age;
                result = get_string_index(&row_set, 0, name, 10);
                if (result != STARDUST_DB_OK) goto error;

                result = get_int_named(&row_set, "age", &age);
                if (result != STARDUST_DB_OK) goto error;

                printf("name: %s, age: %li\n", name, age);

                next_row(&row_set);

                is_end(&row_set, &ended);
            }
        } else {
            printf("Could not mount drive\n");
        }
        goto end;
        error:
        printf("Error code: %d, message: %s\n", result, error);
        end:
        close_row_set(&row_set);
        close_db(&db);
        f_unmount("");

        return NULL;
    } else {
        printf("Couldn't initialise disk\n");
    }
}

int app_main(struct app_main_args *aargs)
{
    create_thread("rust_main", main_thread, UKERNEL_FLAG, aargs->si_info);
    return 0;
}

#endif // RUST_LIB_TEST