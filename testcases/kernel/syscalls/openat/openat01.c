/******************************************************************************
 *
 * Copyright (c) International Business Machines  Corp., 2006
 *  Author: Yi Yang <yyangcdl@cn.ibm.com>
 * Copyright (c) Cyril Hrubis 2014 <chrubis@suse.cz>
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * DESCRIPTION
 *  This test case will verify basic function of openat
 *  added by kernel 2.6.16 or up.
 *
 *****************************************************************************/

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <error.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include "test.h"
#include "safe_macros.h"
#include "lapi/fcntl.h"
#include "openat.h"

static void setup(void);
static void cleanup(void);

char *TCID = "openat01";

static int dirfd, fd;
static int fd_invalid = 100;
static int fd_atcwd = AT_FDCWD;

#define TEST_FILE "test_file"
#define TEST_DIR "test_dir/"

static char glob_path[256];

static struct test_case {
	int *dirfd;
	const char *pathname;
	int exp_ret;
	int exp_errno;
} test_cases[] = {
	{&dirfd, TEST_FILE, 0, 0},
	{&dirfd, glob_path, 0, 0},
	{&fd, TEST_FILE, -1, ENOTDIR},
	{&fd_invalid, TEST_FILE, -1, EBADF},
	{&fd_atcwd, TEST_DIR TEST_FILE, 0, 0}
};

int TST_TOTAL = ARRAY_SIZE(test_cases);

static void verify_openat(struct test_case *test)
{
	TEST(openat(*test->dirfd, test->pathname, O_RDWR, 0600));

	if ((test->exp_ret == -1 && TEST_RETURN != -1) ||
	    (test->exp_ret == 0 && TEST_RETURN < 0)) {
		tst_resm(TFAIL | TTERRNO,
		         "openat() returned %ldl, expected %d",
			 TEST_RETURN, test->exp_ret);
		return;
	}

	if (TEST_RETURN > 0)
		SAFE_CLOSE(cleanup, TEST_RETURN);

	if (TEST_ERRNO != test->exp_errno) {
		tst_resm(TFAIL | TTERRNO,
		         "openat() returned wrong errno, expected %s(%d)",
			 tst_strerrno(test->exp_errno), test->exp_errno);
		return;
	}

	tst_resm(TPASS | TTERRNO, "openat() returned %ld", TEST_RETURN);
}

int main(int ac, char **av)
{
	int lc;
	const char *msg;
	int i;

	if ((msg = parse_opts(ac, av, NULL, NULL)) != NULL)
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);

	setup();

	for (lc = 0; TEST_LOOPING(lc); lc++) {
		tst_count = 0;

		for (i = 0; i < TST_TOTAL; i++)
			verify_openat(test_cases + i);
	}

	cleanup();
	tst_exit();
}

static void setup(void)
{
	char *tmpdir;

	tst_sig(NOFORK, DEF_HANDLER, cleanup);

	tst_tmpdir();

	SAFE_MKDIR(cleanup, TEST_DIR, 0700);
	dirfd = SAFE_OPEN(cleanup, TEST_DIR, O_DIRECTORY);
	fd = SAFE_OPEN(cleanup, TEST_DIR TEST_FILE, O_CREAT | O_RDWR, 0600);

	tmpdir = tst_get_tmpdir();
	snprintf(glob_path, sizeof(glob_path), "%s/" TEST_DIR TEST_FILE,
	         tmpdir);
	free(tmpdir);

	TEST_PAUSE;
}

static void cleanup(void)
{
	if (fd > 0 && close(fd))
		tst_resm(TWARN | TERRNO, "close(fd) failed");

	if (dirfd > 0 && close(dirfd))
		tst_resm(TWARN | TERRNO, "close(dirfd) failed");

	tst_rmdir();
}
