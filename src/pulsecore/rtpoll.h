#ifndef foopulsertpollhfoo
#define foopulsertpollhfoo

/* $Id$ */

/***
  This file is part of PulseAudio.

  Copyright 2004-2006 Lennart Poettering

  PulseAudio is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation; either version 2.1 of the
  License, or (at your option) any later version.

  PulseAudio is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with PulseAudio; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA.
***/

#include <poll.h>
#include <sys/types.h>
#include <limits.h>

#include <pulse/sample.h>
#include <pulsecore/asyncmsgq.h>
#include <pulsecore/fdsem.h>

/* An implementation of a "real-time" poll loop. Basically, this is
 * yet another wrapper around poll(). However it has certain
 * advantages over pa_mainloop and suchlike:
 *
 * 1) It uses timer_create() and POSIX real time signals to guarantee
 * optimal high-resolution timing. Starting with Linux 2.6.21 hrtimers
 * are available, and since right now only nanosleep() and the POSIX
 * clock and timer interfaces have been ported to hrtimers (and not
 * ppoll/pselect!) we have to combine ppoll() with timer_create(). The
 * fact that POSIX timers and POSIX rt signals are used internally is
 * completely hidden.
 *
 * 2) It allows raw access to the pollfd data to users
 *
 * 3) It allows arbitrary functions to be run before entering the
 * actual poll() and after it.
 *
 * Only a single interval timer is supported..*/

typedef struct pa_rtpoll pa_rtpoll;
typedef struct pa_rtpoll_item pa_rtpoll_item;

typedef enum pa_rtpoll_priority {
    PA_RTPOLL_EARLY  = -100,          /* For veeery important stuff, like handling control messages */
    PA_RTPOLL_NORMAL = 0,             /* For normal stuff */
    PA_RTPOLL_LATE   = +100,          /* For housekeeping */
    PA_RTPOLL_NEVER  = INT_MAX,       /* For stuff that doesn't register any callbacks, but only fds to listen on */ 
} pa_rtpoll_priority_t;

pa_rtpoll *pa_rtpoll_new(void);
void pa_rtpoll_free(pa_rtpoll *p);

/* Install the rtpoll in the current thread */
void pa_rtpoll_install(pa_rtpoll *p);

/* Sleep on the rtpoll until the time event, or any of the fd events
 * is triggered. If "wait" is 0 we don't sleep but only update the
 * struct pollfd. Returns negative on error, positive if the loop
 * should continue to run, 0 when the loop should be terminated
 * cleanly. */
int pa_rtpoll_run(pa_rtpoll *f, int wait);

void pa_rtpoll_set_timer_absolute(pa_rtpoll *p, const struct timespec *ts);
void pa_rtpoll_set_timer_periodic(pa_rtpoll *p, pa_usec_t usec);
void pa_rtpoll_set_timer_relative(pa_rtpoll *p, pa_usec_t usec);
void pa_rtpoll_set_timer_disabled(pa_rtpoll *p);

/* A new fd wakeup item for pa_rtpoll */
pa_rtpoll_item *pa_rtpoll_item_new(pa_rtpoll *p, pa_rtpoll_priority_t prio, unsigned n_fds);
void pa_rtpoll_item_free(pa_rtpoll_item *i);

/* Please note that this pointer might change on every call and when
 * pa_rtpoll_run() is called. Hence: call this immediately before
 * using the pointer and don't save the result anywhere */
struct pollfd *pa_rtpoll_item_get_pollfd(pa_rtpoll_item *i, unsigned *n_fds);

/* Set the callback that shall be called when there's time to do some work: If the
 * callback returns a value > 0, the poll is skipped and the next
 * iteraton of the loop will start immediately. */
void pa_rtpoll_item_set_work_callback(pa_rtpoll_item *i, int (*work_cb)(pa_rtpoll_item *i));

/* Set the callback that shall be called immediately before entering
 * the sleeping poll: If the callback returns a value > 0, the poll is
 * skipped and the next iteraton of the loop will start
 * immediately.. */
void pa_rtpoll_item_set_before_callback(pa_rtpoll_item *i, int (*before_cb)(pa_rtpoll_item *i));

/* Set the callback that shall be called immediately after having
 * entered the sleeping poll */
void pa_rtpoll_item_set_after_callback(pa_rtpoll_item *i, void (*after_cb)(pa_rtpoll_item *i));

void pa_rtpoll_item_set_userdata(pa_rtpoll_item *i, void *userdata);
void* pa_rtpoll_item_get_userdata(pa_rtpoll_item *i);

pa_rtpoll_item *pa_rtpoll_item_new_fdsem(pa_rtpoll *p, pa_rtpoll_priority_t prio, pa_fdsem *s);
pa_rtpoll_item *pa_rtpoll_item_new_asyncmsgq(pa_rtpoll *p, pa_rtpoll_priority_t prio, pa_asyncmsgq *q);

/* Requests the loop to exit. Will cause the next iteration of
 * pa_rtpoll_run() to return 0 */
void pa_rtpoll_quit(pa_rtpoll *p);

#endif
