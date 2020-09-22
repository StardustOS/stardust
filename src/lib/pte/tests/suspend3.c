/* Copyright (C) 2017, Ward Jaradat and Jonathan Lewis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * File: suspend3.c
 *
 *
 * --------------------------------------------------------------------------
 *
 *      Copyright(C) 2018 Jon Lewis
 *
 *      Contact Email: jon.lewis@st-andrews.ac.uk
 *
 *
 * --------------------------------------------------------------------------
 *
 * Test Synopsis:
 * - calling pthread_kill(SIGUSR1) on non-existent thread returns ESRCH
 *
 * Test Method (Validation or Falsification):
 * -
 *
 * Requirements Tested:
 * -
 *
 * Features Tested:
 * - Suspend and resume of thread
 *
 * Cases Tested:
 * - parent attempts to supend child that has already exited
 *
 * Description:
 * - parent thread calls pthread_kill(SIGUSR1) on child to suspend it when it has already exited
 *
 * Environment:
 * -
 *
 * Input:
 * - None.
 *
 * Output:
 * - File name, Line number, and failed expression on failure.
 * - No output on success.
 *
 * Assumptions:
 * -
 *
 * Pass Criteria:
 * - Process returns zero exit status.
 *
 * Fail Criteria:
 * - Process returns non-zero exit status.
 */


#include <os/config.h>

#ifdef ENABLE_PTE_TESTS

#include <pte/test.h>
#include <errno.h>


#include <os/trace.h>

static void *mychild(void *arg) {
    unsigned long i;
    for (i = 0; i < 1000; i++) {
    }
    return 0;
}

int pthread_test_suspend_resume3() {
    pthread_t child;
    void *result = 0;

    tprintk("suspend3: parent %i about to create child\n", pthread_self());
    assert(pthread_create(&child, NULL, mychild, NULL) == 0);

    sleep(100); // give child some time to run

    tprintk("suspend3: parent slept and about to suspend child %i\n", child);

    assert(pthread_kill(child, SIGUSR1) == ESRCH);

    sleep(2000); // wait before resumeing child

    tprintk("suspend3: parent slept and about to resume child %i\n", child);
    assert(pthread_kill(child, SIGUSR1) == ESRCH);

    tprintk("suspend3: parent about to join child %i and expect 0-valued result\n", child);
    assert(pthread_join(child, (void **) &result) == 0);
    return 0;
}

#endif
