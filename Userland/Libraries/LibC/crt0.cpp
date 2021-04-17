/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
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

#include <AK/Types.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/internals.h>
#include <unistd.h>

#ifndef _DYNAMIC_LOADER
extern "C" {

extern u32 __stack_chk_guard;

int main(int, char**, char**);

// Tell the compiler that this may be called from somewhere else.
int _start(int argc, char** argv, char** env);

int _start(int argc, char** argv, char** env)
{
    u32 original_stack_chk = __stack_chk_guard;
    arc4random_buf(&__stack_chk_guard, sizeof(__stack_chk_guard));

    if (__stack_chk_guard == 0)
        __stack_chk_guard = original_stack_chk;

    environ = env;
    __environ_is_malloced = false;

    _init();

    int status = main(argc, argv, environ);

    exit(status);

    // We should never get here, but if we ever do, make sure to
    // restore the stack guard to the value we entered _start with.
    // Then we won't trigger the stack canary check on the way out.
    __stack_chk_guard = original_stack_chk;

    return 20150614;
}
}
#endif
