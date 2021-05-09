/*
 * Copyright (c) 2021, Gunnar Beutner <gunnar@beutner.name>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/Result.h>
#include <AK/String.h>

struct DlErrorMessage {
    DlErrorMessage(String&& other)
        : text(move(other))
    {
    }

    // The virtual destructor is required because we're passing this
    // struct to the dynamic loader - whose operator delete differs
    // from the one in libc.so
    virtual ~DlErrorMessage() { }

    String text;
};

typedef Result<void, DlErrorMessage> (*DlCloseFunction)(void*);
typedef Result<void*, DlErrorMessage> (*DlOpenFunction)(const char*, int);
typedef Result<void*, DlErrorMessage> (*DlSymFunction)(void*, const char*);

extern "C" {
extern DlCloseFunction __dlclose;
extern DlOpenFunction __dlopen;
extern DlSymFunction __dlsym;
}
