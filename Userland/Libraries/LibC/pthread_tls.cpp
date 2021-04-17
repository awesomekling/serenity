/*
 * Copyright (c) 2021, the SerenityOS developers.
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

#include <AK/Atomic.h>
#include <LibPthread/pthread.h>
#include <unistd.h>

#ifndef _DYNAMIC_LOADER
extern "C" {

static constexpr int max_keys = PTHREAD_KEYS_MAX;

struct KeyTable {
    KeyDestructor destructors[max_keys] { nullptr };
    int next { 0 };
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};

struct SpecificTable {
    void* values[max_keys] { nullptr };
};

static KeyTable s_keys;

__thread SpecificTable t_specifics;

int __pthread_key_create(pthread_key_t* key, KeyDestructor destructor)
{
    int ret = 0;
    __pthread_mutex_lock(&s_keys.mutex);
    if (s_keys.next >= max_keys) {
        ret = EAGAIN;
    } else {
        *key = s_keys.next++;
        s_keys.destructors[*key] = destructor;
        ret = 0;
    }
    __pthread_mutex_unlock(&s_keys.mutex);
    return ret;
}

int pthread_key_create(pthread_key_t*, KeyDestructor) __attribute__((weak, alias("__pthread_key_create")));

int __pthread_key_delete(pthread_key_t key)
{
    if (key < 0 || key >= max_keys)
        return EINVAL;
    __pthread_mutex_lock(&s_keys.mutex);
    s_keys.destructors[key] = nullptr;
    __pthread_mutex_unlock(&s_keys.mutex);
    return 0;
}

int pthread_key_delete(pthread_key_t) __attribute__((weak, alias("__pthread_key_delete")));

void* __pthread_getspecific(pthread_key_t key)
{
    if (key < 0)
        return nullptr;
    if (key >= max_keys)
        return nullptr;
    return t_specifics.values[key];
}

void* pthread_getspecific(pthread_key_t) __attribute__((weak, alias("__pthread_getspecific")));

int __pthread_setspecific(pthread_key_t key, const void* value)
{
    if (key < 0)
        return EINVAL;
    if (key >= max_keys)
        return EINVAL;

    t_specifics.values[key] = const_cast<void*>(value);
    return 0;
}

int pthread_setspecific(pthread_key_t, const void*) __attribute__((weak, alias("__pthread_setspecific")));

void __pthread_key_destroy_for_current_thread()
{
    // This function will either be called during exit_thread, for a pthread, or
    // during global program shutdown for the main thread.

    __pthread_mutex_lock(&s_keys.mutex);
    size_t num_used_keys = s_keys.next;

    // Dr. POSIX accounts for weird key destructors setting their own key again.
    // Or even, setting other unrelated keys? Odd, but whatever the Doc says goes.

    for (size_t destruct_iteration = 0; destruct_iteration < PTHREAD_DESTRUCTOR_ITERATIONS; ++destruct_iteration) {
        bool any_nonnull_destructors = false;
        for (size_t key_index = 0; key_index < num_used_keys; ++key_index) {
            void* value = exchange(t_specifics.values[key_index], nullptr);

            if (value && s_keys.destructors[key_index]) {
                any_nonnull_destructors = true;
                (*s_keys.destructors[key_index])(value);
            }
        }
        if (!any_nonnull_destructors)
            break;
    }
    __pthread_mutex_unlock(&s_keys.mutex);
}
}
#endif
