/*
 * time.c - Builtin functions that provide time-related functions.
 *
 */

/*
 * Copyright (C) 2012
 * the Free Software Foundation, Inc.
 * 
 * This file is part of GAWK, the GNU implementation of the
 * AWK Programming Language.
 * 
 * GAWK is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * GAWK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "gawkapi.h"

#include "gettext.h"
#define _(msgid)  gettext(msgid)
#define N_(msgid) msgid

static const gawk_api_t *api;	/* for convenience macros to work */
static awk_ext_id_t *ext_id;
static const char *ext_version = "time extension: version 1.0";
static awk_bool_t (*init_func)(void) = NULL;

int plugin_is_GPL_compatible;

#if defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif
#if defined(HAVE_SELECT) && defined(HAVE_SYS_SELECT_H)
#include <sys/select.h>
#endif
#if defined(HAVE_NANOSLEEP) && defined(HAVE_TIME_H)
#include <time.h>
#endif

/*
 * Returns time since 1/1/1970 UTC as a floating point value; should
 * have sub-second precision, but the actual precision will vary based
 * on the platform
 */
static awk_value_t *
do_gettimeofday(int nargs, awk_value_t *result)
{
	double curtime;

	assert(result != NULL);

	if (do_lint && nargs > 0)
		lintwarn(ext_id, _("gettimeofday: ignoring arguments"));

#if defined(HAVE_GETTIMEOFDAY)
	{
		struct timeval tv;
		gettimeofday(&tv,NULL);
		curtime = tv.tv_sec+(tv.tv_usec/1000000.0);
	}
#elif defined(HAVE_GETSYSTEMTIMEASFILETIME)
	/* based on perl win32/win32.c:win32_gettimeofday() implementation */
	{
		union {
			unsigned __int64 ft_i64;
			FILETIME ft_val;
		} ft;

		/* # of 100-nanosecond intervals since January 1, 1601 (UTC) */
		GetSystemTimeAsFileTime(&ft.ft_val);
#ifdef __GNUC__
#define Const64(x) x##LL
#else
#define Const64(x) x##i64
#endif
/* Number of 100 nanosecond units from 1/1/1601 to 1/1/1970 */
#define EPOCH_BIAS  Const64(116444736000000000)
		curtime = (ft.ft_i64 - EPOCH_BIAS)/10000000.0;
#undef Const64
	}
#else
	/* no way to retrieve system time on this platform */
	curtime = -1;
	update_ERRNO_string(_("gettimeofday: not supported on this platform"));
#endif

	return make_number(curtime, result);
}

/*
 * Returns 0 if successful in sleeping the requested time;
 * returns -1 if there is no platform support, or if the sleep request
 * did not complete successfully (perhaps interrupted)
 */
static awk_value_t *
do_sleep(int nargs, awk_value_t *result)
{
	awk_value_t num;
	double secs;
	int rc;

	assert(result != NULL);

	if (do_lint && nargs > 1)
		lintwarn(ext_id, _("sleep: called with too many arguments"));

	if (! get_argument(0, AWK_NUMBER, &num)) {
		update_ERRNO_string(_("sleep: missing required numeric argument"));
		return make_number(-1, result);
	}
	secs = num.num_value;

	if (secs < 0) {
		update_ERRNO_string(_("sleep: argument is negative"));
		return make_number(-1, result);
	}

#if defined(HAVE_NANOSLEEP)
	{
		struct timespec req;

		req.tv_sec = secs;
		req.tv_nsec = (secs-(double)req.tv_sec)*1000000000.0;
		if ((rc = nanosleep(&req,NULL)) < 0)
			/* probably interrupted */
			update_ERRNO_int(errno);
	}
#elif defined(HAVE_SELECT)
	{
		struct timeval timeout;

		timeout.tv_sec = secs;
		timeout.tv_usec = (secs-(double)timeout.tv_sec)*1000000.0;
		if ((rc = select(0,NULL,NULL,NULL,&timeout)) < 0)
			/* probably interrupted */
			update_ERRNO_int(errno);
	}
#else
	/* no way to sleep on this platform */
	rc = -1;
	update_ERRNO_str(_("sleep: not supported on this platform"));
#endif

	return make_number(rc, result);
}

static awk_ext_func_t func_table[] = {
	{ "gettimeofday", do_gettimeofday, 0 },
	{ "sleep", do_sleep, 1 },
};

/* define the dl_load function using the boilerplate macro */

dl_load_func(func_table, time, "")