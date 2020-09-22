#ifndef _BUG_H_
#define _BUG_H_

#include <os/console.h>
#include <os/poweroff.h>

#define BUG_ON(x) do { \
    if (x) {printk("BUG at %s:%d\n", __FILE__, __LINE__); backtrace_and_poweroff(); } \
} while (0)

#define BUG() BUG_ON(1)

#endif
