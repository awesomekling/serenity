/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <syscall.h>
#include <unistd.h>

extern "C" {

mode_t umask(mode_t mask)
{
    return syscall(SC_umask, mask);
}

int mkdir(const char* pathname, mode_t mode)
{
    return mkdirat(AT_FDCWD, pathname, mode);
}

int mkdirat(int dirfd, const char* pathname, mode_t mode)
{
    if (!pathname) {
        errno = EFAULT;
        return -1;
    }
    Syscall::SC_mkdir_params params { dirfd, { pathname, strlen(pathname) }, mode };
    int rc = syscall(SC_mkdir, &params);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int chmod(const char* pathname, mode_t mode)
{
    return fchmodat(AT_FDCWD, pathname, mode, 0);
}

int fchmodat(int dirfd, const char* pathname, mode_t mode, int flags)
{
    if (!pathname) {
        errno = EFAULT;
        return -1;
    }

    Syscall::SC_chmod_params params { dirfd, { pathname, strlen(pathname) }, mode, flags };
    int rc = syscall(SC_chmod, &params);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int fchmod(int fd, mode_t mode)
{
    int rc = syscall(SC_fchmod, fd, mode);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int mkfifo(const char* pathname, mode_t mode)
{
    return mknod(pathname, mode | S_IFIFO, 0);
}

static int do_stat(int dirfd, const char* path, struct stat* statbuf, int flags)
{
    if (!path) {
        errno = EFAULT;
        return -1;
    }
    Syscall::SC_stat_params params { dirfd, { path, strlen(path) }, statbuf, flags };
    int rc = syscall(SC_stat, &params);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int lstat(const char* path, struct stat* statbuf)
{
    return do_stat(AT_FDCWD, path, statbuf, AT_SYMLINK_NOFOLLOW);
}

int stat(const char* path, struct stat* statbuf)
{
    return do_stat(AT_FDCWD, path, statbuf, 0);
}

int fstat(int fd, struct stat* statbuf)
{
    int rc = syscall(SC_fstat, fd, statbuf);
    __RETURN_WITH_ERRNO(rc, rc, -1);
}

int fstatat(int fd, const char* path, struct stat* statbuf, int flags)
{
    return do_stat(fd, path, statbuf, flags);
}
}
