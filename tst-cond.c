/* Copyright (C) 2002, 2010, 2013 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Darren Hart <dvhltc@us.ibm.com>
   Based on pthread_cond_hang.c by Dinakar Guniguntala <dino@in.ibm.com>

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

#define _GNU_SOURCE
#include <error.h>
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#define LOW_PRIO  1
#define MED_PRIO  2
#define HIGH_PRIO 3
#define MAIN_PRIO 4

static pthread_cond_t race_var;
static pthread_mutex_t race_mut;

static pthread_cond_t sig1, sig2, sig3;
static pthread_mutex_t m1, m2, m3;

static volatile unsigned int done = 0;

static void *
low_tf (void *p)
{
  int err;

  /* Wait for do_test to start all the threads.  */
  err = pthread_mutex_lock (&m1);
  if (err != 0)
    error (EXIT_FAILURE, err, "low_tf: failed to lock m1");
  err = pthread_cond_wait (&sig1, &m1);
  if (err != 0)
    error (EXIT_FAILURE, err, "low_tf: cond_wait failed on sig1");

  err = pthread_mutex_lock (&race_mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "low_tf: failed to lock race_mut");

  puts ("low_tf: locked");

  /* Signal the high_tf that we have the race_mut, it will preempt us until it
   * blocks on race_mut.  */
  err = pthread_cond_signal (&sig2);
  if (err != 0)
    error (EXIT_FAILURE, err, "low_tf: failed to signal sig2");

  /* pthread_cond_wait() holds the cond_lock when it unlocks race_mut.  high_tf
     will preempt us before we can release the cond_lock.  It will signal med_tf
     which will continue to block us after high_tf tries to block on race_var if
     it isn't PTHREAD_PRIO_INHERIT, and the cond_lock will never be released.  */
  err = pthread_cond_wait (&race_var, &race_mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "low_tf: cond_wait failed on race_var");

  puts ("low_tf: done waiting");

  err = pthread_mutex_unlock (&race_mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "low_tf: failed to unlock race_mut");

  return NULL;
}

static void *
high_tf (void *p)
{
  int err;

  err = pthread_mutex_lock (&m2);
  if (err != 0)
    error (EXIT_FAILURE, err, "high_tf: failed to lock m2");

  /* Wait for low_tf to take race_mut and signal us.  We will preempt low_tf
   * until we block on race_mut below.  */
  err = pthread_cond_wait (&sig2, &m2);
  if (err != 0)
    error (EXIT_FAILURE, err, "high_tf: cond_wait failed on sig2");

  /* Wait for low_tf to release the lock as it waits on race_var.  */
  err = pthread_mutex_lock (&race_mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "high_tf: failed to lock race_mut");

  puts ("high_tf: locked");

  /* Signal the med_tf to start spinning.  */
  err = pthread_cond_signal (&sig3);
  if (err != 0)
    error (EXIT_FAILURE, err, "high_tf: failed to signal sig3");

  /* If the race_var isn't PTHREAD_PRIO_INHERIT, we will block on the
     race_var cond_lock waiting for the low_tf to release it in it's
     pthread_cond_wait(&race_var, &race_mut) call.  */
  err = pthread_cond_wait (&race_var, &race_mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "high_tf: cond_wait failed on race_var");

  puts ("high_tf: done waiting");

  err = pthread_mutex_unlock (&race_mut);
  if (err != 0)
    error (EXIT_FAILURE, err, "high_tf: failed to unlock race_mut");

  done = 1;
  return NULL;
}

static void *
med_tf (void *p)
{
  int err;

  err = pthread_mutex_lock (&m3);
  if (err != 0)
    error (EXIT_FAILURE, err, "med_tf: failed to lock m3");

  /* Wait for high_tf to signal us.  */
  err = pthread_cond_wait (&sig3, &m3);
  if (err != 0)
    error (EXIT_FAILURE, err, "med_tf: cond_wait failed on sig3");

  puts ("med_tf: spinning");

  while (!done)
          /* Busy wait to block low threads.  */;

  puts ("med_tf: done spinning");

  return NULL;
}

static int
do_test (void)
{
  pthread_t low_thread;
  pthread_t med_thread;
  pthread_t high_thread;
  struct sched_param param;
  pthread_attr_t attr;
  pthread_mutexattr_t m_attr;
  pthread_condattr_t c_attr;
  cpu_set_t cset;

  int err;


  /* Initialize mutexes and condvars.  */

  err = pthread_attr_init (&attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init pthread_attr");
  err = pthread_attr_setinheritsched (&attr, PTHREAD_EXPLICIT_SCHED);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set attr inheritsched");
  err = pthread_attr_setschedpolicy (&attr, SCHED_FIFO);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set attr schedpolicy");

  err = pthread_condattr_init (&c_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init condattr");

  err = pthread_cond_init (&sig1, &c_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init cond sig1");
  err = pthread_cond_init (&sig2, &c_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init cond sig2");
  err = pthread_cond_init (&sig3, &c_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init cond sig3");
  err = pthread_cond_init (&race_var, &c_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init cond race_var");

  err = pthread_mutexattr_init (&m_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init mutexattr");
  err = pthread_mutexattr_setprotocol (&m_attr, PTHREAD_PRIO_INHERIT);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set mutexattr protocol");
  err = pthread_mutex_init (&m1, &m_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init mutex m1");
  err = pthread_mutex_init (&m2, &m_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init mutex m2");
  err = pthread_mutex_init (&m3, &m_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init mutex m3");
  err = pthread_mutex_init (&race_mut, &m_attr);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to init mutex race_mut");

  /* Setup scheduling parameters and create threads.  */
  param.sched_priority = MAIN_PRIO;
  err = sched_setscheduler (0, SCHED_FIFO, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set scheduler policy");

  CPU_ZERO (&cset);
  CPU_SET (0, &cset);
  err = sched_setaffinity (0, sizeof (cpu_set_t), &cset);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set CPU affinity");

  param.sched_priority = LOW_PRIO;
  err = pthread_attr_setschedparam (&attr, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set sched param for low_tf");
  err = pthread_create (&low_thread, &attr, low_tf, (void*)NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to create low_tf");

  param.sched_priority = MED_PRIO;
  err = pthread_attr_setschedparam (&attr, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set sched param for med_tf");
  pthread_create (&med_thread, &attr, med_tf, (void*)NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to create med_tf");

  param.sched_priority = HIGH_PRIO;
  err = pthread_attr_setschedparam (&attr, &param);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to set sched param for high_tf");
  err = pthread_create (&high_thread, &attr, high_tf, (void*)NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to create high_tf");

  /* Wait for the threads to start and block on their respective condvars.  */
  usleep (1000);
  err = pthread_cond_signal (&sig1);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to signal condition");

  /* Wake low_tf and high_tf, allowing them to complete.  If race_var is not
     PTHREAD_PRIO_INHERIT, low_tf will not have released the race_var cond_lock
     and neither thread will have waited on race_var.  */
  usleep (1000);
  err = pthread_cond_broadcast (&race_var);
  if (err != 0)
    error (EXIT_FAILURE, err, "parent: failed to broadcast condition");

  /* Wait for threads to complete.  */
  err = pthread_join (low_thread, (void**) NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "join of low_tf failed");
  err = pthread_join (med_thread, (void**) NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "join of med_tf failed");
  err = pthread_join (high_thread, (void**) NULL);
  if (err != 0)
    error (EXIT_FAILURE, err, "join of high_tf failed");

  puts ("done");

  return 0;
}

int main(void)
{
	do_test();
	return 0;
}
