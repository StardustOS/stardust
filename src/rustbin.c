#ifdef RUST_BIN
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

int main(int argc, char *argv[]);

static void* main_thread(void* a) {
    FATFS FatFs;
    if (init_disk()) {
        while (!blk_has_initialised()) {}
        if (f_mount(&FatFs, "", 1) == FR_OK) {
            printf("Starting Rust binary\n");
            int result = main(0, NULL);
            printf("Exited rust binary with code %d\n", result);
        } else {
            printf("Could not mount drive\n");
        }
        f_unmount("");
    } else {
        printf("Couldn't initialise disk\n");
    }
    return NULL;
}

int app_main(struct app_main_args *aargs)
{
    create_thread("rust_main", main_thread, UKERNEL_FLAG, aargs->si_info);
    return 0;
}


#endif