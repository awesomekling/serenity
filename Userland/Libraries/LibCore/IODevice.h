/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
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

    int error() const { return m_error; }
    const char* error_string() const;

    bool has_error() const { return m_error != 0; }

    int read(u8* buffer, int length);

    ByteBuffer read(size_t max_size);
    ByteBuffer read_all();
    String read_line(size_t max_size = 16384);

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
