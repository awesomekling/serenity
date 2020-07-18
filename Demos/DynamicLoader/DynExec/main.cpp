/*
 * Copyright (c) 2019-2020, Andrew Kaster <andrewdkaster@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <Kernel/Syscall.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int g_lib_var1;
extern int g_lib_var2;
// extern __thread int g_tls_lib_var;
extern __thread int g_tls_lib_var2;

const char* g_string = "Hello, World!\n";

int libfunc();
int libfunc2();
// int libfunc_tls();
void local_dbgputstr(const char* str, int len);

int main(int, char**)
{
    local_dbgputstr(g_string, 15);
    int sum = 0;
    sum += libfunc() + g_lib_var1 + g_lib_var2;
    sum += libfunc2();
    local_dbgputstr("1\n", 2);
    sum += g_tls_lib_var2;
    // sum += libfunc_tls();
    local_dbgputstr("2\n", 2);
    // sum += g_tls_lib_var + g_tls_lib_var2;
    local_dbgputstr("3\n", 2);
    printf("ho ho!\n");
    open("/does/not/exist", 0);
    perror("open");
    open("/etc/passwd", O_RDWR);
    perror("open");

    return sum;
}
