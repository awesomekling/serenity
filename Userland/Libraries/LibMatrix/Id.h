/*
 * Copyright (c) 2021, Max Wipfli <max.wipfli@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/String.h>
#include <AK/Traits.h>

namespace Matrix {

class UserId {
public:
    UserId() = default;
    explicit UserId(String value)
        : m_value(move(value))
    {
        VERIFY(is_valid(m_value));
    }

    String const& value() const { return m_value; }
    StringView local_part() const { return m_value.substring_view(1, m_value.find(':').value() - 1); }
    StringView home_server() const { return m_value.substring_view(m_value.find(':').value() + 1); }

    bool operator==(UserId const& other) const
    {
        return m_value == other.m_value;
    }

    bool operator==(String const& other) const
    {
        return m_value == other;
    }

    static bool is_valid(StringView const& value)
    {
        return !value.is_empty() && value[0] == '@' && value.contains(':');
    }

private:
    String m_value;
};

}

namespace AK {

template<>
struct Formatter<Matrix::UserId> : Formatter<StringView> {
    void format(FormatBuilder& builder, Matrix::UserId const& value)
    {
        Formatter<StringView>::format(builder, value.value());
    }
};

template<>
struct Traits<Matrix::UserId> : public GenericTraits<Matrix::UserId> {
    static unsigned hash(const Matrix::UserId& id) { return id.value().hash(); }
};

}
