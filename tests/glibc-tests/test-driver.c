/* Main function for test programs.
   Copyright (C) 2016-2019 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* This file should be included from test cases.  It will define a
   main function which provides the test wrapper.

   It assumes that the test case defines a function

     int do_test (void);

   and arranges for that function being called under the test wrapper.
   The do_test function should return 0 to indicate a passing test, 1
   to indicate a failing test, or 77 to indicate an unsupported test.
   Other result values could be used to indicate a failing test, but
   the result of the expression is passed to exit and exit only
   returns the lower 8 bits of its input.  A non-zero return with some
   values could cause a test to incorrectly be considered passing when
   it really failed.  For this reason, the function should always
   return 0 (EXIT_SUCCESS), 1 (EXIT_FAILURE), or 77
   (EXIT_UNSUPPORTED).

   The test function may print out diagnostic or warning messages as well
   as messages about failures.  These messages should be printed to stdout
   and not stderr so that the output is properly ordered with respect to
   the rest of the glibc testsuite run output.

   Several preprocessors macros can be defined before including this
   file.

   The name of the do_test function can be changed with the
   TEST_FUNCTION macro.  It must expand to the desired function name.

   If the test case needs access to command line parameters, it must
   define the TEST_FUNCTION_ARGV macro with the name of the test
   function.  It must have the following type:

     int TEST_FUNCTION_ARGV (int argc, char **argv);

   This overrides the do_test default function and is incompatible
   with the TEST_FUNCTION macro.

   If PREPARE is defined, it must expand to the name of a function of
   the type

     void PREPARE (int argc, char **);

   This function will be called early, after parsing the command line,
   but before running the test, in the parent process which acts as
   the test supervisor.

   If CLEANUP_HANDLER is defined, it must expand to the name of a
   function of the type

     void CLEANUP_HANDLER (void);

   This function will be called from the timeout (SIGALRM) signal
   handler.

   If TIMEOUT is defined, it must be positive constant.  It overrides
   the default test timeout and is measured in seconds.
*/
#include "test-driver.h"

#include <assert.h>
#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <malloc.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

/* The PID of the test process.  */
static pid_t test_pid;

static void *delayed_exit_thread(void *seconds_as_ptr)
{
	int seconds = (uintptr_t) seconds_as_ptr;
	struct timespec delay = { seconds, 0 };
	struct timespec remaining = { 0 };
	int err;

	err = nanosleep(&delay, &remaining);
	if (err != 0)
		error(EXIT_FAILURE, err,
		      "delayed_exit_thread: nanosleep failed");

	/* Exit the process sucessfully.  */
	exit(0);
	return NULL;
}

void delayed_exit(int seconds)
{
	/* Create the new thread with all signals blocked.  */
	sigset_t all_blocked;
	sigfillset(&all_blocked);
	sigset_t old_set;
	pthread_t thr;
	int err;

	err = pthread_sigmask(SIG_SETMASK, &all_blocked, &old_set);
	if (err != 0)
		error(EXIT_FAILURE, err,
		      "delayed_exit: failed to set signal mask");

	/* Create a detached thread. */

	err = pthread_create(&thr, NULL, delayed_exit_thread,
			     (void *)(uintptr_t) seconds);
	if (err != 0)
		error(EXIT_FAILURE, err,
		      "delayed_exit: failed to create delayed thread");

	err = pthread_detach(thr);
	if (err != 0)
		error(EXIT_FAILURE, err,
		      "delayed_exit: failed to detach thread");

	/* Restore the original signal mask.  */
	err = pthread_sigmask(SIG_SETMASK, &old_set, NULL);
	if (err != 0)
		error(EXIT_FAILURE, err,
		      "delayed_exit: failed to set signal mask");
}

/* The cleanup handler passed to test_main.  */
static void (*cleanup_function) (void);

/* Timeout handler.  We kill the child and exit with an error.  */
static void
    __attribute__ ((noreturn))
    signal_handler(int sig)
{
	int killed;
	int status;
	int i;

	assert(test_pid > 1);
	/* Kill the whole process group.  */
	kill(-test_pid, SIGKILL);
	/* In case setpgid failed in the child, kill it individually too.  */
	kill(test_pid, SIGKILL);

	/* Wait for it to terminate.  */
	for (i = 0; i < 5; ++i) {
		killed = waitpid(test_pid, &status, WNOHANG | WUNTRACED);
		if (killed != 0)
			break;

		/* Delay, give the system time to process the kill.  If the
		   nanosleep() call return prematurely, all the better.  We
		   won't restart it since this probably means the child process
		   finally died.  */
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 100000000;
		nanosleep(&ts, NULL);
	}
	if (killed != 0 && killed != test_pid) {
		printf("Failed to kill test process: %m\n");
		exit(1);
	}

	if (cleanup_function != NULL)
		cleanup_function();

	if (sig == SIGINT) {
		signal(sig, SIG_DFL);
		raise(sig);
	}

	if (killed == 0 || (WIFSIGNALED(status) && WTERMSIG(status) == SIGKILL))
		puts("Timed out: killed the child process");
	else if (WIFSTOPPED(status))
		printf("Timed out: the child process was %s\n",
		       strsignal(WSTOPSIG(status)));
	else if (WIFSIGNALED(status))
		printf("Timed out: the child process got signal %s\n",
		       strsignal(WTERMSIG(status)));
	else
		printf("Timed out: killed the child process but it exited %d\n",
		       WEXITSTATUS(status));

	/* Exit with an error.  */
	exit(1);
}

/* Run test_function or test_function_argv.  */
static int
run_test_function(int argc, char **argv, const struct test_config *config)
{
	if (config->test_function != NULL)
		return config->test_function();
	else if (config->test_function_argv != NULL)
		return config->test_function_argv(argc, argv);
	else {
		printf("error: no test function defined\n");
		exit(1);
	}
}

static bool test_main_called;
const char *test_dir = NULL;

static int support_test_main(int argc, char **argv, const struct test_config *config)
{
	pid_t termpid;
	int status;

	if (test_main_called) {
		printf("error: test_main called for a second time\n");
		exit(1);
	}
	test_main_called = true;

	cleanup_function = config->cleanup_function;

	if (mallopt != NULL)
		mallopt(M_PERTURB, 42);

	/* Set TMPDIR to specified test directory.  */
	test_dir = getenv("TMPDIR");
	if (test_dir == NULL || test_dir[0] == '\0')
		test_dir = "/tmp";

	int timeout = config->timeout;
	if (timeout == 0)
		timeout = DEFAULT_TIMEOUT;

	/* Make sure we see all message, even those on stdout.  */
	setvbuf(stdout, NULL, _IONBF, 0);

	/* Call the initializing function, if one is available.  */
	if (config->prepare_function != NULL)
		config->prepare_function(argc, argv);

	test_pid = fork();
	if (test_pid == 0) {
		/* We put the test process in its own pgrp so that if it bogusly
		   generates any job control signals, they won't hit the whole
		   build.  */
		if (setpgid(0, 0) != 0)
			printf("Failed to set the process group ID: %m\n");

		/* Execute the test function and exit with the return value. */
		exit(run_test_function(argc, argv, config));
	} else if (test_pid < 0) {
		printf("Cannot fork test program: %m\n");
		exit(1);
	}

	/* Set timeout.  */
	signal(SIGALRM, signal_handler);
	alarm(timeout);

	/* Make sure we clean up if the wrapper gets interrupted.  */
	signal(SIGINT, signal_handler);

	/* Wait for the regular termination.  */
	do {
		termpid = waitpid(test_pid, &status, 0);
	} while (termpid < 0 && errno == EINTR);

	if (termpid == -1) {
		printf("Waiting for test program failed: %m\n");
		exit(1);
	}
	if (termpid != test_pid) {
		printf
		    ("Oops, wrong test program terminated: expected %ld, got %ld\n",
		     (long int)test_pid, (long int)termpid);
		exit(1);
	}

	/* Process terminated normaly without timeout etc.  */
	if (WIFEXITED(status)) {
		/* Exit with the return value of the test.  */
		return WEXITSTATUS(status);
	} else {
		/* Process was killed by timer or other signal.  */
		printf("Didn't expect signal from child: got `%s'\n",
		       strsignal(WTERMSIG(status)));
		exit(1);
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct test_config test_config;
	memset(&test_config, 0, sizeof(test_config));

#ifdef PREPARE
	test_config.prepare_function = (PREPARE);
#endif

#if defined (TEST_FUNCTION) && defined (TEST_FUNCTON_ARGV)
#error TEST_FUNCTION and TEST_FUNCTION_ARGV cannot be defined at the same time
#endif
#if defined (TEST_FUNCTION)
	test_config.test_function = TEST_FUNCTION;
#elif defined (TEST_FUNCTION_ARGV)
	test_config.test_function_argv = TEST_FUNCTION_ARGV;
#else
	test_config.test_function = do_test;
#endif

#ifdef CLEANUP_HANDLER
	test_config.cleanup_function = CLEANUP_HANDLER;
#endif

#ifdef TIMEOUT
	test_config.timeout = TIMEOUT;
#endif

	return support_test_main(argc, argv, &test_config);
}
