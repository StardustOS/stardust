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
 * File: suspend1.c
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
 * - calling pthread_kill(SIGUSR1) suspends a thread if not currently suspended and resumes a thread if currently suspended
 * -  We need very basic signal handling for non-zero SIGUSR1 signal in pthread_kill to support JamVM
 * which uses SIGUSR1 to suspend and wake up all pthreads (implementing Java threads) prior to and
 * after garbage collection
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
 * - self-supension of child and thread-resume by parent
 *
 * Description:
 * - child thread calls pthread_kill(SIGUSR1) on itself to suspend itself and parent thread calls pthread_kill(SIGUSR1) on child to wake it and perform join
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
  tprintk("suspend1: child %i running and about to suspend self ...\n", pthread_self());
  assert(pthread_kill(pthread_self(), SIGUSR1) == 0); // suspend self
  tprintk("suspend1: child %i running again after being resumed ...\n", pthread_self());
    return 0;
}

int pthread_test_suspend_resume1() {
  pthread_t child;
  void *result = 0;

  tprintk("suspend1: parent %i about to create child\n", pthread_self());
  assert(pthread_create(&child, NULL, mychild, NULL) == 0);

  sleep(3000); // give child time to run

  tprintk("suspend1: parent slept and about to resume child %i\n", child);

  assert(pthread_kill(child, SIGUSR1) == 0);

  tprintk("suspend1: parent about to join child %i and expect 0-valued result\n", child);
  assert(pthread_join(child, (void **) &result) == 0);
  return 0;
}

#endif
