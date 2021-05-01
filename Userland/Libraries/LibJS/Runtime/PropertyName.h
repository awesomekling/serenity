/*
 * Copyright (c) 2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/FlyString.h>
#include <LibJS/Runtime/StringOrSymbol.h>

namespace JS {

class PropertyName {
public:
    enum class Type {
        Invalid,
        Number,
        String,
        Symbol,
    };

    static PropertyName from_value(GlobalObject& global_object, Value value)
    {
        if (value.is_empty())
            return {};
        if (value.is_symbol())
            return &value.as_symbol();
        if (value.is_integer() && value.as_i32() >= 0)
            return value.as_i32();
        auto string = value.to_string(global_object);
        if (string.is_null())
            return {};
        return string;
    }

    PropertyName() { }

    PropertyName(i32 index)
        : m_type(Type::Number)
        , m_number(index)
    {
        VERIFY(index >= 0);
    }

    PropertyName(const char* chars)
        : m_type(Type::String)
        , m_string(FlyString(chars))
    {
    }

    PropertyName(const String& string)
        : m_type(Type::String)
        , m_string(FlyString(string))
    {
        VERIFY(!string.is_null());
    }

    PropertyName(const FlyString& string)
        : m_type(Type::String)
        , m_string(string)
    {
        VERIFY(!string.is_null());
    }

    PropertyName(Symbol* symbol)
        : m_type(Type::Symbol)
        , m_symbol(symbol)
    {
        VERIFY(symbol);
    }

    PropertyName(const StringOrSymbol& string_or_symbol)
    {
        if (string_or_symbol.is_string()) {
            m_string = string_or_symbol.as_string();
            m_type = Type::String;
        } else if (string_or_symbol.is_symbol()) {
            m_symbol = const_cast<Symbol*>(string_or_symbol.as_symbol());
            m_type = Type::Symbol;
        }
    }

    bool is_valid() const { return m_type != Type::Invalid; }
    bool is_number() const { return m_type == Type::Number; }
    bool is_string() const { return m_type == Type::String; }
    bool is_symbol() const { return m_type == Type::Symbol; }

    u32 as_number() const
    {
        VERIFY(is_number());
        return m_number;
    }

    const FlyString& as_string() const
    {
        VERIFY(is_string());
        return m_string;
    }

    const Symbol* as_symbol() const
    {
        VERIFY(is_symbol());
        return m_symbol;
    }

    String to_string() const
    {
        VERIFY(is_valid());
        VERIFY(!is_symbol());
        if (is_string())
            return as_string();
        return String::number(as_number());
    }

    StringOrSymbol to_string_or_symbol() const
    {
        VERIFY(is_valid());
        VERIFY(!is_number());
        if (is_string())
            return StringOrSymbol(as_string());
        return StringOrSymbol(as_symbol());
    }

    Value to_value(VM& vm) const
    {
        if (is_string())
            return js_string(vm, m_string);
        if (is_number())
            return Value(m_number);
        if (is_symbol())
            return m_symbol;
        return js_undefined();
    }

private:
    Type m_type { Type::Invalid };
    FlyString m_string;
    Symbol* m_symbol { nullptr };
    u32 m_number { 0 };
};

}
