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

#include <os/config.h>

#ifdef ENABLE_PTE_TESTS
#include <pte/osal.h>
#include <pte/test.h>

const char * error_string;

int assertE;

#define RUN_THREAD_TESTS
#define RUN_MISC_TESTS
#define RUN_MUTEX_TESTS
#define RUN_SEM_TESTS
#define RUN_SPIN_TESTS
#define RUN_CANCEL_TESTS
#define RUN_CLEANUP_TESTS
#define RUN_BARRIER_TESTS
#define RUN_CONDVAR_TESTS
#define RUN_RWLOCK_TESTS
#define RUN_TSD_TEST_1
#define RUN_TSD_TEST_2
//#define RUN_SUSPEND_TESTS // these tests have been added by Jon Lewis

/* The following tests cannot be run in series with other tests as they rely on knowing what is on the reuse queue */

//#define RUN_PRIORITY_TEST_1
//#define RUN_PRIORITY_TEST_2
//#define RUN_INHERIT_TEST
//#define RUN_REUSE_TEST_1
//#define RUN_REUSE_TEST_2

#ifdef RUN_THREAD_TESTS
static void runThreadTests(void) {

	printf("Create test #1\n");
	pthread_test_create1();

	printf("Create test #2\n");
	pthread_test_create2();

	printf("Create test #3\n");
	pthread_test_create3();

	printf("Join test #0\n");
	pthread_test_join0();

	printf("Join test #1\n");
	pthread_test_join1();

	printf("Join test #2\n");
	pthread_test_join2();

	printf("Join test #3\n");
	pthread_test_join3();

	printf("Join test #4\n");
	pthread_test_join4();

	printf("Kill test #1\n");
	pthread_test_kill1();

	printf("Exit test #1\n");
	pthread_test_exit1();

	printf("Exit test #2\n");
	pthread_test_exit2();

	printf("Exit test #3\n");
	pthread_test_exit3();

	printf("Exit test #4\n");
	pthread_test_exit4();

	printf("Exit test #5\n");
	pthread_test_exit5();

}
#endif

#ifdef RUN_REUSE_TEST_1
static void runReuseTest1(void) {

	printf("Reuse test #1\n");
	pthread_test_reuse1();
}
#endif

#ifdef RUN_REUSE_TEST_2
static void runReuseTest2(void) {

	printf("Reuse test #2\n");
	pthread_test_reuse2();
}
#endif

#ifdef RUN_PRIORITY_TEST_1
static void runPriorityTest1(void) {

	printf("Priority test #1\n");
	pthread_test_priority1();
}
#endif

#ifdef RUN_PRIORITY_TEST_2
static void runPriorityTest2(void) {

	printf("Priority test #2\n");
	pthread_test_priority2();
}
#endif

#ifdef RUN_INHERIT_TEST
static void runInheritTest(void) {

	printf("Inherit test #1\n");
	pthread_test_inherit1();
}
#endif

#ifdef RUN_MISC_TESTS
static void runMiscTests(void) {

	printf("Valid test #1\n");
	pthread_test_valid1();

	printf("Valid test #2\n");
	pthread_test_valid2();

	printf("Self test #1\n");
	pthread_test_self1();

	printf("Self test #2\n");
	pthread_test_self2();

	printf("Equal test #1\n");
	pthread_test_equal1();

	printf("Count test #1\n");
	pthread_test_count1();

	printf("Delay test #1\n");
	pthread_test_delay1();

	printf("Delay test #2\n");
	pthread_test_delay2();

	printf("Once test #1\n");
	pthread_test_once1();

	printf("Once test #2\n");
	pthread_test_once2();

	printf("Once test #3\n");
	pthread_test_once3();

	printf("Once test #4\n");
	pthread_test_once4();

	printf("Errno test #1\n");
	pthread_test_errno1();

	printf("Detach test #1\n");
	pthread_test_detach1();

}
#endif

#ifdef RUN_MUTEX_TESTS
static void runMutexTests(void) {

	printf("Mutex test #1\n");
	pthread_test_mutex1();

	printf("Mutex test #1(e)\n");
	pthread_test_mutex1e();

	printf("Mutex test #1(n)\n");
	pthread_test_mutex1n();

	printf("Mutex test #1(r)\n");
	pthread_test_mutex1r();

	printf("Mutex test #2\n");
	pthread_test_mutex2();

	printf("Mutex test #2(e)\n");
	pthread_test_mutex2e();

	printf("Mutex test #2(r)\n");
	pthread_test_mutex2r();

	printf("Mutex test #3\n");
	pthread_test_mutex3();

	printf("Mutex test #3(e)\n");
	pthread_test_mutex3e();

	printf("Mutex test #3(r)\n");
	pthread_test_mutex3r();

	printf("Mutex test #4\n");
	pthread_test_mutex4();

	printf("Mutex test #5\n");
	pthread_test_mutex5();

	printf("Mutex test #6\n");
	pthread_test_mutex6();

	printf("Mutex test #6e\n");
	pthread_test_mutex6e();

	printf("Mutex test #6es\n");
	pthread_test_mutex6es();

	printf("Mutex test #6n\n");
	pthread_test_mutex6n();

	printf("Mutex test #6r\n");
	pthread_test_mutex6r();

	printf("Mutex test #6rs\n");
	pthread_test_mutex6rs();

	printf("Mutex test #6s\n");
	pthread_test_mutex6s();

	printf("Mutex test #7\n");
	pthread_test_mutex7();

	printf("Mutex test #7e\n");
	pthread_test_mutex7e();

	printf("Mutex test #7n\n");
	pthread_test_mutex7n();

	printf("Mutex test #7r\n");
	pthread_test_mutex7r();

	printf("Mutex test #8\n");
	pthread_test_mutex8();

	printf("Mutex test #8e\n");
	pthread_test_mutex8e();

	printf("Mutex test #8n\n");
	pthread_test_mutex8n();

	printf("Mutex test #8r\n");
	pthread_test_mutex8r();
}
#endif

#ifdef RUN_BARRIER_TESTS
static void runBarrierTests(void) {

	printf("Barrier test #1\n");
	pthread_test_barrier1();

	printf("Barrier test #2\n");
	pthread_test_barrier2();

	printf("Barrier test #3\n");
	pthread_test_barrier3();

	printf("Barrier test #4\n");
	pthread_test_barrier4();

	printf("Barrier test #5\n");
	pthread_test_barrier5();
}
#endif

#ifdef RUN_SEM_TESTS
static void runSemTests(void) {

	printf("Semaphore test #1\n");
	pthread_test_semaphore1();

	printf("Semaphore test #2\n");
	pthread_test_semaphore2();

	printf("Semaphore test #3\n");
	pthread_test_semaphore3();

	printf("Semaphore test #4\n");
	pthread_test_semaphore4();

	printf("Semaphore test #4t\n");
	pthread_test_semaphore4t();

	printf("Semaphore test #5\n");
	pthread_test_semaphore5();

	printf("Semaphore test #6\n");
	pthread_test_semaphore6();

}
#endif

#ifdef RUN_SPIN_TESTS
static void runSpinTests() {

	printf("Spin test #1\n");
	pthread_test_spin1();

	printf("Spin test #2\n");
	pthread_test_spin2();

	printf("Spin test #3\n");
	pthread_test_spin3();

	printf("Spin test #4\n");
	pthread_test_spin4();

}
#endif

#ifdef RUN_CONDVAR_TESTS
static void runCondvarTests() {

	printf("Condvar test #1\n");
	pthread_test_condvar1();

	printf("Condvar test #1-1\n");
	pthread_test_condvar1_1();

	printf("Condvar test #1-2\n");
	pthread_test_condvar1_2();

	printf("Condvar test #2\n");
	pthread_test_condvar2();

	printf("Condvar test #2-0\n");
	pthread_test_condvar2_0();

	printf("Condvar test #2-1\n");
	pthread_test_condvar2_1();

	printf("Condvar test #3\n");
	pthread_test_condvar3();

	printf("Condvar test #3-1\n");
	pthread_test_condvar3_1();

	printf("Condvar test #3-2\n");
	pthread_test_condvar3_2();

	printf("Condvar test #3-3\n");
	pthread_test_condvar3_3();

	printf("Condvar test #4\n");
	pthread_test_condvar4();

	printf("Condvar test #5\n");
	pthread_test_condvar5();

	printf("Condvar test #6\n");
	pthread_test_condvar6();

	printf("Condvar test #7\n");
	pthread_test_condvar7();

	printf("Condvar test #8\n");
	pthread_test_condvar8();

	printf("Condvar test #9\n");
	pthread_test_condvar9();

}
#endif

#ifdef RUN_RWLOCK_TESTS
static void runRwlockTests() {

	printf("Rwlock test #1\n");
	pthread_test_rwlock1();

	printf("Rwlock test #2\n");
	pthread_test_rwlock2();

	printf("Rwlock test #2t\n");
	pthread_test_rwlock2t();

	printf("Rwlock test #3\n");
	pthread_test_rwlock3();

	printf("Rwlock test #3t\n");
	pthread_test_rwlock3t();

	printf("Rwlock test #4\n");
	pthread_test_rwlock4();

	printf("Rwlock test #4t\n");
	pthread_test_rwlock4t();

	printf("Rwlock test #5\n");
	pthread_test_rwlock5();

	printf("Rwlock test #5t\n");
	pthread_test_rwlock5t();

	printf("Rwlock test #6\n");
	pthread_test_rwlock6();

	printf("Rwlock test #6t \n");
	pthread_test_rwlock6t();

	printf("Rwlock test #6t2\n");
	pthread_test_rwlock6t2();

	printf("Rwlock test #7\n");
	pthread_test_rwlock7();

	printf("Rwlock test #8\n");
	pthread_test_rwlock8();

}
#endif

#ifdef RUN_CLEANUP_TESTS
static void runCleanupTests() {

    printf("Cleanup test #0\n");
    pthread_test_cleanup0();

    printf("Cleanup test #1\n");
    pthread_test_cleanup1();

    printf("Cleanup test #2\n");
    pthread_test_cleanup2();

    printf("Cleanup test #3\n");
    pthread_test_cleanup3();
}
#endif

#ifdef RUN_CANCEL_TESTS
static void runCancelTests() {

	printf("Cancel test #1\n");
	pthread_test_cancel1();

	printf("Cancel test #2\n");
	pthread_test_cancel2();

	printf("Cancel test #3\n");
	pthread_test_cancel3();

	printf("Cancel test #4\n");
	pthread_test_cancel4();

	printf("Cancel test #5\n");
	pthread_test_cancel5();

	printf("Cancel test #6a\n");
	pthread_test_cancel6a();

	printf("Cancel test #6d\n");
	pthread_test_cancel6d();
}
#endif

#ifdef RUN_TSD_TEST_1
static void runTsdTest1() {

	printf("TSD test #1\n");
	pthread_test_tsd1();
}
#endif

#ifdef RUN_TSD_TEST_2
static void runTsdTest2() {

	printf("TSD test #2\n");
	pthread_test_tsd2();
}
#endif

#ifdef RUN_SUSPEND_TESTS
static void runSuspendtests(void) {
	printf("Suspend test #1\n");
	pthread_test_suspend_resume1();

    printf("Suspend test #2\n");
    pthread_test_suspend_resume2();

    printf("Suspend test #3\n");
    pthread_test_suspend_resume3();

    printf("Suspend test #4\n");
    pthread_test_suspend_resume4();

    printf("Suspend test #5\n");
    pthread_test_suspend_resume5();
}
#endif

void *pte_test_main(void *p){

	pthread_init();

#ifdef RUN_THREAD_TESTS
	runThreadTests();				// passed
#endif

#ifdef RUN_MISC_TESTS
	runMiscTests();					// passed
#endif

#ifdef RUN_MUTEX_TESTS
	runMutexTests();				// passed
#endif

#ifdef RUN_SEM_TESTS
	runSemTests();				// passed except 2 tests which may be incorrectly written
#endif

#ifdef RUN_SPIN_TESTS
	runSpinTests();				// passed
#endif

#ifdef RUN_CLEANUP_TESTS
    runCleanupTests();
#endif

#ifdef RUN_CANCEL_TESTS
	runCancelTests();				// passed
#endif

#ifdef RUN_CONDVAR_TESTS
	runCondvarTests();
#endif


	/* The following tests are related to functions which are not required by the jvm */

#ifdef RUN_BARRIER_TESTS
	runBarrierTests();
#endif

#ifdef RUN_RWLOCK_TESTS
	runRwlockTests();
#endif

#ifdef RUN_TSD_TEST_1
	runTsdTest1();
#endif

#ifdef RUN_TSD_TEST_2
	runTsdTest2();
#endif

#ifdef RUN_SUSPEND_TESTS
    runSuspendtests();
#endif

#ifdef RUN_REUSE_TEST_1
	runReuseTest1();			// passed
#endif

#ifdef RUN_REUSE_TEST_2
	runReuseTest2();			// passed
#endif

#ifdef RUN_PRIORITY_TEST_1
	runPriorityTest1();			// passed
#endif

#ifdef RUN_PRIORITY_TEST_2
	runPriorityTest2();
#endif

#ifdef RUN_INHERIT_TEST
	runInheritTest();
#endif

	printf("Tests complete!\n");

	// Print the list of threads in the system
	sched_print_threads();
	return NULL;
}


#endif