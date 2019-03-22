/* Interfaces for the test driver.
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

#ifndef SUPPORT_TEST_DRIVER_H
#define SUPPORT_TEST_DRIVER_H

#include <stdlib.h>
#include <sys/cdefs.h>

__BEGIN_DECLS struct test_config {
	void (*prepare_function) (int argc, char **argv);
	int (*test_function) (void);
	int (*test_function_argv) (int argc, char **argv);
	void (*cleanup_function) (void);
	const void *options;	/* Custom options if not NULL.  */
	int timeout;		/* Test timeout in seconds.  */
};

enum {
	/* Test exit status which indicates that the feature is
	   unsupported. */
	EXIT_UNSUPPORTED = 77,

	/* Default timeout is twenty seconds.  Tests should normally
	   complete faster than this, but if they don't, that's abnormal
	   (a bug) anyways.  */
	DEFAULT_TIMEOUT = 20,
};

#define SKIP_TEST(reason)					\
	do {							\
		printf("test skipped due to: %s\n", reason);	\
		exit(EXIT_UNSUPPORTED);				\
	} while(0);

/* The directory the test should use for temporary files.  */
extern const char *test_dir;

__END_DECLS
#endif /* SUPPORT_TEST_DRIVER_H */
