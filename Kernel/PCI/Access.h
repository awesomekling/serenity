/*
 * Copyright (c) 2020, Liav A. <liavalb@hotmail.co.il>
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

#pragma once

#include <AK/String.h>
#include <Kernel/PCI/Definitions.h>

namespace Kernel {

class PCI::Access {
public:
    virtual void enumerate_all(Function<void(Address, ID)>&) = 0;

    void enumerate_bus(int type, u8 bus, Function<void(Address, ID)>&);
    void enumerate_functions(int type, u8 bus, u8 slot, u8 function, Function<void(Address, ID)>& callback);
    void enumerate_slot(int type, u8 bus, u8 slot, Function<void(Address, ID)>& callback);

    static Access& the();
    static bool is_initialized();
    virtual uint32_t get_segments_count() = 0;
    virtual uint8_t get_segment_start_bus(u32 segment) = 0;
    virtual uint8_t get_segment_end_bus(u32 segment) = 0;
    virtual String get_access_type() = 0;

    virtual void write8_field(Address address, u32 field, u8 value) = 0;
    virtual void write16_field(Address address, u32 field, u16 value) = 0;
    virtual void write32_field(Address address, u32 field, u32 value) = 0;

    virtual u8 read8_field(Address address, u32 field) = 0;
    virtual u16 read16_field(Address address, u32 field) = 0;
    virtual u32 read32_field(Address address, u32 field) = 0;

protected:
    Access();
};

}
