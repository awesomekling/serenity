/*
 * Copyright (c) 2020, Emanuel Sprung <emanuel.sprung@gmail.com>
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

#include "RegexError.h"
#include "RegexLexer.h"
#include "RegexOptions.h"
#include <AK/Forward.h>
#include <AK/Types.h>
#include <AK/Vector.h>

namespace AK {
namespace regex {

#define ENUMERATE_OPCODES                          \
    __ENUMERATE_OPCODE(Compare)                    \
    __ENUMERATE_OPCODE(Jump)                       \
    __ENUMERATE_OPCODE(ForkJump)                   \
    __ENUMERATE_OPCODE(ForkStay)                   \
    __ENUMERATE_OPCODE(SaveLeftCaptureGroup)       \
    __ENUMERATE_OPCODE(SaveRightCaptureGroup)      \
    __ENUMERATE_OPCODE(SaveLeftNamedCaptureGroup)  \
    __ENUMERATE_OPCODE(SaveRightNamedCaptureGroup) \
    __ENUMERATE_OPCODE(CheckBegin)                 \
    __ENUMERATE_OPCODE(CheckEnd)                   \
    __ENUMERATE_OPCODE(Exit)

enum class OpCode : u8 {
#define __ENUMERATE_OPCODE(x) x,
    ENUMERATE_OPCODES
#undef __ENUMERATE_OPCODE
};

enum class CharacterCompareType : u8 {
    Undefined = 0,
    Inverse,
    AnySingleCharacter,
    OrdinaryCharacter,
    OrdinaryCharacters,
    CharacterClass,
    RangeExpression,
    RangeExpressionDummy,
};

enum class CharacterClass : u8 {
    Alnum = 0,
    Cntrl,
    Lower,
    Space,
    Alpha,
    Digit,
    Print,
    Upper,
    Blank,
    Graph,
    Punct,
    Xdigit,
};

class ByteCodeValue {
public:
    union CompareValue {
        CompareValue(const CharacterClass value)
            : character_class(value)
        {
        }
        CompareValue(const char value1, const char value2)
            : range_values { value1, value2 }
        {
        }
        const CharacterClass character_class;
        const struct {
            const char from;
            const char to;
        } range_values;
    };

    // FIXME: implement to<T> method and use only one type to store data
    union {
        const OpCode op_code;
        const char* string;
        const char ch;
        const int number;
        const size_t positive_number;
        const CompareValue compare_value;
        const CharacterCompareType compare_type;
    };

    const char* name() const;
    static const char* name(OpCode);

    ByteCodeValue(const OpCode value)
        : op_code(value)
    {
    }
    ByteCodeValue(const char* value)
        : string(value)
    {
    }
    ByteCodeValue(const char value)
        : ch(value)
    {
    }
    ByteCodeValue(const int value)
        : number(value)
    {
    }
    ByteCodeValue(const size_t value)
        : positive_number(value)
    {
    }
    ByteCodeValue(const CharacterClass value)
        : compare_value(value)
    {
    }
    ByteCodeValue(const char value1, const char value2)
        : compare_value(value1, value2)
    {
    }
    ByteCodeValue(const CharacterCompareType value)
        : compare_type(value)
    {
    }

    ~ByteCodeValue() = default;
};

struct CompareTypeAndValuePair {
    CharacterCompareType type;
    ByteCodeValue value;
};

struct ParserResult {
    Vector<ByteCodeValue> bytecode;
    size_t capture_groups_count;
    size_t named_capture_groups_count;
    size_t match_length_minimum;
    Error error;
    Token error_token;
};

template<typename T>
struct GenericParserTraits {
    using OptionsType = T;
};

template<typename T>
struct ParserTraits : public GenericParserTraits<T> {
};

template<>
struct ParserTraits<PosixExtendedParser> : public GenericParserTraits<PosixOptions> {
};

template<typename OptionsType>
class Parser {
public:
    explicit Parser(Lexer& lexer)
        : m_parser_state(lexer)
    {
    }

    Parser(Lexer& lexer, Optional<OptionsType> regex_options)
        : m_parser_state(lexer, regex_options)
    {
    }

    virtual ~Parser() = default;

    ParserResult parse(Optional<OptionsType> regex_options = {});
    bool has_error() const { return m_parser_state.error != Error::NoError; }
    Error error() const { return m_parser_state.error; }

protected:
    virtual bool parse_internal(Vector<ByteCodeValue>&, size_t& match_length_minimum) = 0;

    bool match(TokenType type) const;
    bool match(char ch) const;
    Token consume();
    Token consume(TokenType type, Error error);
    bool consume(const String&);
    void reset();
    bool done() const;

    bool set_error(Error error);

    void insert_bytecode_compare_values(Vector<ByteCodeValue>&, Vector<CompareTypeAndValuePair>&&);
    void insert_bytecode_group_capture_left(Vector<ByteCodeValue>& stack);
    void insert_bytecode_group_capture_right(Vector<ByteCodeValue>& stack);
    void insert_bytecode_group_capture_left(Vector<ByteCodeValue>& stack, const StringView& name);
    void insert_bytecode_group_capture_right(Vector<ByteCodeValue>& stack, const StringView& name);
    void insert_bytecode_alternation(Vector<ByteCodeValue>& stack, Vector<ByteCodeValue>&&, Vector<ByteCodeValue>&&);
    void insert_bytecode_repetition_min_max(Vector<ByteCodeValue>& bytecode_to_repeat, size_t minimum, Optional<size_t> maximum);
    void insert_bytecode_repetition_n(Vector<ByteCodeValue>& stack, Vector<ByteCodeValue>& bytecode_to_repeat, size_t n);
    void insert_bytecode_repetition_min_one(Vector<ByteCodeValue>& bytecode_to_repeat, bool greedy);
    void insert_bytecode_repetition_any(Vector<ByteCodeValue>& bytecode_to_repeat, bool greedy);
    void insert_bytecode_repetition_zero_or_one(Vector<ByteCodeValue>& bytecode_to_repeat, bool greedy);

    struct ParserState {
        Lexer& lexer;
        Token current_token;
        Error error = Error::NoError;
        Token error_token { TokenType::Eof, 0, StringView(nullptr) };
        Vector<ByteCodeValue> bytecode;
        size_t capture_groups_count { 0 };
        size_t named_capture_groups_count { 0 };
        size_t match_length_minimum { 0 };
        OptionsType regex_options;
        explicit ParserState(Lexer& lexer)
            : lexer(lexer)
            , current_token(lexer.next())
        {
        }
        explicit ParserState(Lexer& lexer, Optional<OptionsType> regex_options)
            : lexer(lexer)
            , current_token(lexer.next())
            , regex_options(regex_options.value_or({}))
        {
        }
    };

    ParserState m_parser_state;
};

class PosixExtendedParser final : public Parser<PosixOptions> {
public:
    explicit PosixExtendedParser(Lexer& lexer)
        : Parser(lexer) {};
    PosixExtendedParser(Lexer& lexer, Optional<typename ParserTraits<PosixExtendedParser>::OptionsType> regex_options)
        : Parser(lexer, regex_options) {};
    ~PosixExtendedParser() = default;

private:
    bool match_repetition_symbol();
    bool match_ordinary_characters();

    bool parse_internal(Vector<ByteCodeValue>&, size_t&) override;

    bool parse_root(Vector<ByteCodeValue>&, size_t&);
    bool parse_sub_expression(Vector<ByteCodeValue>&, size_t&);
    bool parse_bracket_expression(Vector<ByteCodeValue>&, size_t&);
    bool parse_repetition_symbol(Vector<ByteCodeValue>&, size_t&);
};

using PosixExtended = PosixExtendedParser;
}
}

using AK::regex::ParserResult;
using AK::regex::PosixExtended;
