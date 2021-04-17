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

#pragma once

#include <AK/Forward.h>
#include <LibCore/Object.h>

namespace Core {

// This is not necessarily a valid iterator in all contexts,
// if we had concepts, this would be InputIterator, not Copyable, Movable.
class LineIterator {
    AK_MAKE_NONCOPYABLE(LineIterator);

public:
    explicit LineIterator(IODevice&, bool is_end = false);

    bool operator==(const LineIterator& other) const { return &other == this || (at_end() && other.is_end()) || (other.at_end() && is_end()); }
    bool is_end() const { return m_is_end; }
    bool at_end() const;

    LineIterator& operator++();

    StringView operator*() const { return m_buffer; }

private:
    NonnullRefPtr<IODevice> m_device;
    bool m_is_end { false };
    String m_buffer;
};

class IODevice : public Object {
    C_OBJECT_ABSTRACT(IODevice)
public:
    enum OpenMode {
        NotOpen = 0,
        ReadOnly = 1,
        WriteOnly = 2,
        ReadWrite = 3,
        Append = 4,
        Truncate = 8,
        MustBeNew = 16,
    };

    virtual ~IODevice() override;

    int fd() const { return m_fd; }
    unsigned mode() const { return m_mode; }
    bool is_open() const { return m_mode != NotOpen; }
    bool eof() const { return m_eof; }
    bool is_readable() const { return mode() & OpenMode::ReadOnly; }
    bool is_writable() const { return mode() & OpenMode::WriteOnly; }

    int error() const { return m_error; }
    const char* error_string() const;

    bool has_error() const { return m_error != 0; }

    int read(u8* buffer, int length);

    ByteBuffer read(size_t max_size);
    ByteBuffer read_all();
    String read_line(size_t max_size = 16384);

    size_t copy_from(const IODevice& source, size_t buffer_size = 65536);

    bool write(const u8*, int size);
    bool write(const StringView&);

    bool truncate(off_t);

    bool can_read_line() const;

    bool can_read() const;

    enum class SeekMode {
        SetPosition,
        FromCurrentPosition,
        FromEndPosition,
    };

    bool seek(i64, SeekMode = SeekMode::SetPosition, off_t* = nullptr);

    virtual bool open(IODevice::OpenMode) = 0;
    virtual bool close();

    int printf(const char*, ...);

    LineIterator line_begin() & { return LineIterator(*this); }
    LineIterator line_end() { return LineIterator(*this, true); }

protected:
    explicit IODevice(Object* parent = nullptr);

    void set_fd(int);
    void set_mode(OpenMode mode) { m_mode = mode; }
    void set_error(int error) const { m_error = error; }
    void set_eof(bool eof) const { m_eof = eof; }

    virtual void did_update_fd(int) { }

private:
    bool populate_read_buffer() const;
    bool can_read_from_fd() const;

    int m_fd { -1 };
    OpenMode m_mode { NotOpen };
    mutable int m_error { 0 };
    mutable bool m_eof { false };
    mutable Vector<u8> m_buffered_data;
};

}
