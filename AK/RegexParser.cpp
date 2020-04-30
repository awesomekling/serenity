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

#include "RegexParser.h"
#include <AK/String.h>
#include <AK/StringBuilder.h>
#include <cstdio>

namespace AK {
namespace regex {

const char* ByteCodeValue::name(OpCode type)
{
    switch (type) {
#define __ENUMERATE_OPCODE(x) \
    case OpCode::x:           \
        return #x;
        ENUMERATE_OPCODES
#undef __ENUMERATE_OPCODE
    default:
        ASSERT_NOT_REACHED();
        return "<Unknown>";
    }
}

const char* ByteCodeValue::name() const
{
    return name(op_code);
}

template<typename OptionsType>
bool Parser<OptionsType>::set_error(Error error)
{
    if (m_parser_state.error == Error::NoError) {
        m_parser_state.error = error;
        m_parser_state.error_token = m_parser_state.current_token;
    }
    return false; // always return false, that eases the API usage (return set_error(...)) :^)
}

template<typename OptionsType>
bool Parser<OptionsType>::done() const
{
    return match(TokenType::Eof);
}

template<typename OptionsType>
bool Parser<OptionsType>::match(TokenType type) const
{
    return m_parser_state.current_token.type() == type;
}

template<typename OptionsType>
Token Parser<OptionsType>::consume()
{
    auto old_token = m_parser_state.current_token;
    m_parser_state.current_token = m_parser_state.lexer.next();
    return old_token;
}

template<typename OptionsType>
Token Parser<OptionsType>::consume(TokenType type, Error error)
{
    if (m_parser_state.current_token.type() != type) {
        set_error(error);
#ifdef __serenity__
        dbg() << "[PARSER] Error: Unexpected token " << m_parser_state.current_token.name() << ". Expected: " << Token::name(type);
#else
        fprintf(stderr, "[PARSER] Error: Unexpected token %s. Expected %s\n", m_parser_state.current_token.name(), Token::name(type));
#endif
    }
    return consume();
}

template<typename OptionsType>
bool Parser<OptionsType>::consume(const String& str)
{
    size_t potentially_go_back { 1 };
    for (auto ch : str) {
        if (match(TokenType::OrdinaryCharacter)) {
            if (m_parser_state.current_token.value()[0] != ch) {
                m_parser_state.lexer.back(potentially_go_back);
                m_parser_state.current_token = m_parser_state.lexer.next();
                return false;
            }
        } else {
            m_parser_state.lexer.back(potentially_go_back);
            m_parser_state.current_token = m_parser_state.lexer.next();
            return false;
        }
        consume(TokenType::OrdinaryCharacter, Error::NoError);
        ++potentially_go_back;
    }
    return true;
}

template<typename OptionsType>
void Parser<OptionsType>::reset()
{
    m_parser_state.bytecode.clear();
    m_parser_state.lexer.reset();
    m_parser_state.current_token = m_parser_state.lexer.next();
    m_parser_state.error = Error::NoError;
    m_parser_state.error_token = { TokenType::Eof, 0, StringView(nullptr) };
    m_parser_state.regex_options = {};
}

template<typename OptionsType>
ParserResult Parser<OptionsType>::parse(Optional<OptionsType> regex_options)
{
    reset();
    if (regex_options.has_value())
        m_parser_state.regex_options = regex_options.value();
    if (parse_internal(m_parser_state.bytecode, m_parser_state.match_length_minimum))
        consume(TokenType::Eof, Error::InvalidPattern);
    else
        set_error(Error::InvalidPattern);

#ifdef REGEX_DEBUG
    printf("[PARSER] Produced bytecode with %lu entries (opcodes + arguments)\n", m_parser_state.bytecode.size());
#endif
    return {
        move(m_parser_state.bytecode),
        move(m_parser_state.capture_groups_count),
        move(m_parser_state.named_capture_groups_count),
        move(m_parser_state.match_length_minimum),
        move(m_parser_state.error),
        move(m_parser_state.error_token)
    };
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_compare_values(Vector<ByteCodeValue>& stack, Vector<CompareTypeAndValuePair>&& pairs)
{
    Vector<ByteCodeValue> bytecode;

    bytecode.empend(OpCode::Compare);
    bytecode.empend(pairs.size()); // number of arguments

    for (auto& value : pairs) {
        ASSERT(value.type != CharacterCompareType::RangeExpressionDummy);
        ASSERT(value.type != CharacterCompareType::Undefined);
        ASSERT(value.type != CharacterCompareType::OrdinaryCharacters);

        bytecode.append(move(value.type));
        if (value.type != CharacterCompareType::Inverse && value.type != CharacterCompareType::AnySingleCharacter)
            bytecode.append(move(value.value));
    }

    stack.append(move(bytecode));
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_group_capture_left(Vector<ByteCodeValue>& stack)
{
    stack.empend(OpCode::SaveLeftCaptureGroup);
    stack.empend(m_parser_state.capture_groups_count);
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_group_capture_left(Vector<ByteCodeValue>& stack, const StringView& name)
{
    stack.empend(OpCode::SaveLeftNamedCaptureGroup);
    stack.empend(name.characters_without_null_termination());
    stack.empend(name.length());
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_group_capture_right(Vector<ByteCodeValue>& stack)
{
    stack.empend(OpCode::SaveRightCaptureGroup);
    stack.empend(m_parser_state.capture_groups_count);
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_group_capture_right(Vector<ByteCodeValue>& stack, const StringView& name)
{
    stack.empend(OpCode::SaveRightNamedCaptureGroup);
    stack.empend(name.characters_without_null_termination());
    stack.empend(name.length());
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_alternation(Vector<ByteCodeValue>& stack, Vector<ByteCodeValue>&& left, Vector<ByteCodeValue>&& right)
{

    // FORKSTAY _ALT
    // REGEXP ALT1
    // JUMP  _END
    // LABEL _ALT
    // REGEXP ALT2
    // LABEL _END

    stack.empend(OpCode::ForkJump);
    stack.empend(left.size() + 2); // Jump to the _ALT label

    for (auto& op : left)
        stack.append(move(op));

    stack.empend(OpCode::Jump);
    stack.empend(right.size()); // Jump to the _END label

    // LABEL _ALT = bytecode.size() + 2

    for (auto& op : right)
        stack.append(move(op));

    // LABEL _END = alterantive_bytecode.size
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_repetition_min_max(Vector<ByteCodeValue>& bytecode_to_repeat, size_t minimum, Optional<size_t> maximum)
{
    Vector<ByteCodeValue> new_bytecode;
    insert_bytecode_repetition_n(new_bytecode, bytecode_to_repeat, minimum);

    if (maximum.has_value()) {
        if (maximum.value() > minimum) {
            auto diff = maximum.value() - minimum;
            new_bytecode.empend(OpCode::ForkStay);
            new_bytecode.empend(diff * (bytecode_to_repeat.size() + 2)); // Jump to the _END label

            for (size_t i = 0; i < diff; ++i) {
                new_bytecode.append(bytecode_to_repeat);
                new_bytecode.empend(OpCode::ForkStay);
                new_bytecode.empend((diff - i - 1) * (bytecode_to_repeat.size() + 2)); // Jump to the _END label
            }
        }
    } else {
        // no maximum value set, repeat finding if possible
        new_bytecode.empend(OpCode::ForkJump);
        new_bytecode.empend(-bytecode_to_repeat.size() - 2); // Jump to the last iteration
    }

    bytecode_to_repeat = move(new_bytecode);
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_repetition_n(Vector<ByteCodeValue>& stack, Vector<ByteCodeValue>& bytecode_to_repeat, size_t n)
{
    for (size_t i = 0; i < n; ++i)
        stack.append(bytecode_to_repeat);
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_repetition_min_one(Vector<ByteCodeValue>& bytecode_to_repeat, bool greedy)
{
    // LABEL _START = -bytecode_to_repeat.size()
    // REGEXP
    // FORKJUMP _START  (FORKSTAY -> Greedy)

    if (greedy)
        bytecode_to_repeat.empend(OpCode::ForkStay);
    else
        bytecode_to_repeat.empend(OpCode::ForkJump);

    bytecode_to_repeat.empend(-bytecode_to_repeat.size() - 1); // Jump to the _START label
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_repetition_any(Vector<ByteCodeValue>& bytecode_to_repeat, bool greedy)
{
    // LABEL _START
    // FORKSTAY _END  (FORKJUMP -> Greedy)
    // REGEXP
    // JUMP  _START
    // LABEL _END

    // LABEL _START = stack.size();
    Vector<ByteCodeValue> bytecode;

    if (greedy)
        bytecode.empend(OpCode::ForkJump);
    else
        bytecode.empend(OpCode::ForkStay);

    bytecode.empend(bytecode_to_repeat.size() + 2); // Jump to the _END label

    for (auto& op : bytecode_to_repeat)
        bytecode.append(move(op));

    bytecode.empend(OpCode::Jump);
    bytecode.empend(-bytecode.size() - 1); // Jump to the _START label
    // LABEL _END = bytecode.size()

    bytecode_to_repeat = move(bytecode);
}

template<typename OptionsType>
void Parser<OptionsType>::insert_bytecode_repetition_zero_or_one(Vector<ByteCodeValue>& bytecode_to_repeat, bool greedy)
{
    // FORKSTAY _END (FORKJUMP -> Greedy)
    // REGEXP
    // LABEL _END
    Vector<ByteCodeValue> bytecode;

    if (greedy)
        bytecode.empend(OpCode::ForkJump);
    else
        bytecode.empend(OpCode::ForkStay);

    bytecode.empend(bytecode_to_repeat.size()); // Jump to the _END label

    for (auto& op : bytecode_to_repeat)
        bytecode.append(move(op));
    // LABEL _END = bytecode.size()

    bytecode_to_repeat = move(bytecode);
}

// =============================
// PosixExtended Parser
// =============================

bool PosixExtendedParser::parse_internal(Vector<ByteCodeValue>& stack, size_t& match_length_minimum)
{
    return parse_root(stack, match_length_minimum);
}

bool PosixExtendedParser::match_repetition_symbol()
{
    auto type = m_parser_state.current_token.type();
    return (type == TokenType::Asterisk
        || type == TokenType::Plus
        || type == TokenType::Questionmark
        || type == TokenType::LeftCurly);
}

bool PosixExtendedParser::match_ordinary_characters()
{
    // NOTE: This method must not be called during bracket and repetition parsing!
    // FIXME: Add assertion for that?
    auto type = m_parser_state.current_token.type();
    return (type == TokenType::OrdinaryCharacter
        || type == TokenType::Comma
        || type == TokenType::Slash
        || type == TokenType::EqualSign
        || type == TokenType::HyphenMinus
        || type == TokenType::Colon);
}

bool PosixExtendedParser::parse_repetition_symbol(Vector<ByteCodeValue>& bytecode_to_repeat, size_t& match_length_minimum)
{
    if (match(TokenType::LeftCurly)) {
        consume();

        StringBuilder number_builder;
        bool ok;

        while (match(TokenType::OrdinaryCharacter)) {
            number_builder.append(consume().value());
        }

        size_t minimum = number_builder.build().to_uint(ok);
        if (!ok)
            return set_error(Error::InvalidBraceContent);

        match_length_minimum *= minimum;

        if (match(TokenType::Comma)) {
            consume();
        } else {
            Vector<ByteCodeValue> bytecode;
            insert_bytecode_repetition_n(bytecode, bytecode_to_repeat, minimum);
            bytecode_to_repeat = move(bytecode);

            consume(TokenType::RightCurly, Error::MismatchingBrace);
            return !has_error();
        }

        Optional<size_t> maximum {};
        number_builder.clear();
        while (match(TokenType::OrdinaryCharacter)) {
            number_builder.append(consume().value());
        }
        if (!number_builder.is_empty()) {
            maximum = number_builder.build().to_uint(ok);
            if (!ok || minimum > maximum.value())
                return set_error(Error::InvalidBraceContent);
        }

        insert_bytecode_repetition_min_max(bytecode_to_repeat, minimum, maximum);

        consume(TokenType::RightCurly, Error::MismatchingBrace);
        return !has_error();

    } else if (match(TokenType::Plus)) {
        consume();

        bool greedy = match(TokenType::Questionmark);
        if (greedy)
            consume();

        // Note: dont touch match_length_minimum, it's already correct
        insert_bytecode_repetition_min_one(bytecode_to_repeat, greedy);
        return !has_error();

    } else if (match(TokenType::Asterisk)) {
        consume();
        match_length_minimum = 0;

        bool greedy = match(TokenType::Questionmark);
        if (greedy)
            consume();

        insert_bytecode_repetition_any(bytecode_to_repeat, greedy);

        return !has_error();

    } else if (match(TokenType::Questionmark)) {
        consume();
        match_length_minimum = 0;

        bool greedy = match(TokenType::Questionmark);
        if (greedy)
            consume();

        insert_bytecode_repetition_zero_or_one(bytecode_to_repeat, greedy);
        return !has_error();
    }

    return false;
}

bool PosixExtendedParser::parse_bracket_expression(Vector<ByteCodeValue>& stack, size_t& match_length_minimum)
{
    Vector<CompareTypeAndValuePair> values;

    for (;;) {

        if (match(TokenType::HyphenMinus)) {
            consume();

            if (values.is_empty() || (values.size() == 1 && values.last().type == CharacterCompareType::Inverse)) {
                // first in the bracket expression
                values.append({ CharacterCompareType::OrdinaryCharacter, { '-' } });
            } else if (match(TokenType::RightBracket)) {
                // Last in the bracket expression
                values.append({ CharacterCompareType::OrdinaryCharacter, { '-' } });
            } else if (values.last().type == CharacterCompareType::OrdinaryCharacter) {
                values.append({ CharacterCompareType::RangeExpressionDummy, 0 });

                if (match(TokenType::HyphenMinus)) {
                    consume();
                    // Valid range, add ordinary character
                    values.append({ CharacterCompareType::OrdinaryCharacter, { '-' } });
                }
            } else {
                return set_error(Error::InvalidRange);
            }

        } else if (match(TokenType::OrdinaryCharacter) || match(TokenType::Period) || match(TokenType::Asterisk) || match(TokenType::EscapeSequence) || match(TokenType::Plus)) {
            values.append({ CharacterCompareType::OrdinaryCharacter, { *consume().value().characters_without_null_termination() } });

        } else if (match(TokenType::Circumflex)) {
            auto t = consume();

            if (values.is_empty())
                values.append({ CharacterCompareType::Inverse, 0 });
            else
                values.append({ CharacterCompareType::OrdinaryCharacter, { *t.value().characters_without_null_termination() } });

        } else if (match(TokenType::LeftBracket)) {
            consume();

            if (match(TokenType::Period)) {
                consume();

                // FIXME: Parse collating element, this is needed when we have locale support
                //        This could have impact on length parameter, I guess.
                ASSERT_NOT_REACHED();

                consume(TokenType::Period, Error::InvalidCollationElement);
                consume(TokenType::RightBracket, Error::MismatchingBracket);

            } else if (match(TokenType::EqualSign)) {
                consume();
                // FIXME: Parse collating element, this is needed when we have locale support
                //        This could have impact on length parameter, I guess.
                ASSERT_NOT_REACHED();

                consume(TokenType::EqualSign, Error::InvalidCollationElement);
                consume(TokenType::RightBracket, Error::MismatchingBracket);

            } else if (match(TokenType::Colon)) {
                consume();

                CharacterClass ch_class;
                // parse character class
                if (match(TokenType::OrdinaryCharacter)) {
                    if (consume("alnum"))
                        ch_class = CharacterClass::Alnum;
                    else if (consume("alpha"))
                        ch_class = CharacterClass::Alpha;
                    else if (consume("blank"))
                        ch_class = CharacterClass::Blank;
                    else if (consume("cntrl"))
                        ch_class = CharacterClass::Cntrl;
                    else if (consume("digit"))
                        ch_class = CharacterClass::Digit;
                    else if (consume("graph"))
                        ch_class = CharacterClass::Graph;
                    else if (consume("lower"))
                        ch_class = CharacterClass::Lower;
                    else if (consume("print"))
                        ch_class = CharacterClass::Print;
                    else if (consume("punct"))
                        ch_class = CharacterClass::Punct;
                    else if (consume("space"))
                        ch_class = CharacterClass::Space;
                    else if (consume("upper"))
                        ch_class = CharacterClass::Upper;
                    else if (consume("xdigit"))
                        ch_class = CharacterClass::Xdigit;
                    else
                        return set_error(Error::InvalidCharacterClass);

                    values.append({ CharacterCompareType::CharacterClass, (size_t)ch_class });

                } else
                    return set_error(Error::InvalidCharacterClass);

                // FIXME: we do not support locale specific character classes until locales are implemented

                consume(TokenType::Colon, Error::InvalidCharacterClass);
                consume(TokenType::RightBracket, Error::MismatchingBracket);
            }
        } else if (match(TokenType::RightBracket)) {

            if (values.is_empty() || (values.size() == 1 && values.last().type == CharacterCompareType::Inverse)) {
                // handle bracket as ordinary character
                values.append({ CharacterCompareType::OrdinaryCharacter, { *consume().value().characters_without_null_termination() } });
            } else {
                // closing bracket expression
                break;
            }
        } else
            // nothing matched, this is a failure, as at least the closing bracket must match...
            return set_error(Error::MismatchingBracket);

        // check if range expression has to be completed...
        if (values.size() >= 3 && values.at(values.size() - 2).type == CharacterCompareType::RangeExpressionDummy) {
            if (values.last().type != CharacterCompareType::OrdinaryCharacter)
                return set_error(Error::InvalidRange);

            auto value2 = values.take_last();
            values.take_last(); // RangeExpressionDummy
            auto value1 = values.take_last();

            values.append({ CharacterCompareType::RangeExpression, ByteCodeValue { value1.value.ch, value2.value.ch } });
        }
    }

    if (values.size())
        match_length_minimum = 1;

    if (values.first().type == CharacterCompareType::Inverse)
        match_length_minimum = 0;

    insert_bytecode_compare_values(stack, move(values));

    return !has_error();
}

bool PosixExtendedParser::parse_sub_expression(Vector<ByteCodeValue>& stack, size_t& match_length_minimum)
{
    Vector<ByteCodeValue> bytecode;
    size_t length = 0;
    bool should_parse_repetition_symbol { false };

    for (;;) {
        if (match_ordinary_characters()) {
            Token start_token = m_parser_state.current_token;
            Token last_token = m_parser_state.current_token;
            for (;;) {
                if (!match_ordinary_characters())
                    break;
                ++length;
                last_token = consume();
            }

            if (length > 1) {
                stack.empend(OpCode::Compare);
                stack.empend(1ul); // number of arguments
                stack.empend(CharacterCompareType::OrdinaryCharacters);
                stack.empend(start_token.value().characters_without_null_termination());
                stack.empend(length - ((match_repetition_symbol() && length > 1) ? 1 : 0)); // last character is inserted into 'bytecode' for duplication symbol handling
            }

            if ((match_repetition_symbol() && length > 1) || length == 1) // Create own compare opcode for last character before duplication symbol
                insert_bytecode_compare_values(bytecode, { { CharacterCompareType::OrdinaryCharacter, { last_token.value().characters_without_null_termination()[0] } } });

            should_parse_repetition_symbol = true;
            break;
        }

        if (match_repetition_symbol())
            return set_error(Error::InvalidRepetitionMarker);

        if (match(TokenType::Period)) {
            length = 1;
            consume();
            insert_bytecode_compare_values(bytecode, { { CharacterCompareType::AnySingleCharacter, { 0 } } });
            should_parse_repetition_symbol = true;
            break;
        }

        if (match(TokenType::EscapeSequence)) {
            length = 1;
            Token t = consume();
#ifdef REGEX_DEBUG
            printf("[PARSER] EscapeSequence with substring %s\n", String(t.value()).characters());
#endif

            insert_bytecode_compare_values(bytecode, { { CharacterCompareType::OrdinaryCharacter, { (char)t.value().characters_without_null_termination()[1] } } });
            should_parse_repetition_symbol = true;
            break;
        }

        if (match(TokenType::LeftBracket)) {
            consume();

            Vector<ByteCodeValue> sub_ops;
            if (!parse_bracket_expression(sub_ops, length) || !sub_ops.size())
                return set_error(Error::InvalidBracketContent);

            bytecode.append(move(sub_ops));

            consume(TokenType::RightBracket, Error::MismatchingBracket);
            should_parse_repetition_symbol = true;
            break;
        }

        if (match(TokenType::RightBracket)) {
            return set_error(Error::MismatchingBracket);
        }

        if (match(TokenType::RightCurly)) {
            return set_error(Error::MismatchingBrace);
        }

        if (match(TokenType::Circumflex)) {
            consume();
            bytecode.empend(OpCode::CheckBegin);
            break;
        }

        if (match(TokenType::Dollar)) {
            consume();
            bytecode.empend(OpCode::CheckEnd);
            break;
        }

        if (match(TokenType::RightParen))
            return false;

        if (match(TokenType::LeftParen)) {
            consume();
            Optional<StringView> capture_group_name;
            bool prevent_capture_group = false;
            if (match(TokenType::Questionmark)) {
                consume();

                if (match(TokenType::Colon)) {
                    consume();
                    prevent_capture_group = true;
                } else if (consume("<")) { // named capturing group

                    Token start_token = m_parser_state.current_token;
                    Token last_token = m_parser_state.current_token;
                    size_t capture_group_name_length = 0;
                    for (;;) {
                        if (!match_ordinary_characters())
                            return set_error(Error::InvalidNameForCaptureGroup);
                        if (match(TokenType::OrdinaryCharacter) && m_parser_state.current_token.value()[0] == '>') {
                            consume();
                            break;
                        }
                        ++capture_group_name_length;
                        last_token = consume();
                    }
                    capture_group_name = StringView(start_token.value().characters_without_null_termination(), capture_group_name_length);

                } else if (match(TokenType::EqualSign)) { // positive lookahead
                    consume();
                    ASSERT_NOT_REACHED();
                } else if (consume("!")) { // negative lookahead
                    ASSERT_NOT_REACHED();
                } else if (consume("<")) {
                    if (match(TokenType::EqualSign)) { // positive lookbehind
                        consume();
                        ASSERT_NOT_REACHED();
                    }
                    if (consume("!")) // negative lookbehind
                        ASSERT_NOT_REACHED();
                } else {
                    return set_error(Error::InvalidRepetitionMarker);
                }
            }

            if (!(m_parser_state.regex_options & AllFlags::NoSubExpressions || prevent_capture_group)) {
                if (capture_group_name.has_value())
                    insert_bytecode_group_capture_left(bytecode, capture_group_name.value());
                else
                    insert_bytecode_group_capture_left(bytecode);
            }

            Vector<ByteCodeValue> capture_group_bytecode;

            if (!parse_root(capture_group_bytecode, length))
                return set_error(Error::InvalidPattern);

            bytecode.append(move(capture_group_bytecode));

            consume(TokenType::RightParen, Error::MismatchingParen);

            if (!(m_parser_state.regex_options & AllFlags::NoSubExpressions || prevent_capture_group)) {
                if (capture_group_name.has_value()) {
                    insert_bytecode_group_capture_right(bytecode, capture_group_name.value());
                    ++m_parser_state.named_capture_groups_count;
                } else {
                    insert_bytecode_group_capture_right(bytecode);
                    ++m_parser_state.capture_groups_count;
                }
            }
            should_parse_repetition_symbol = true;
            break;
        }

        return false;
    }

    if (match_repetition_symbol()) {
        if (should_parse_repetition_symbol)
            parse_repetition_symbol(bytecode, length);
        else
            return set_error(Error::InvalidRepetitionMarker);
    }

    stack.append(move(bytecode));
    match_length_minimum += length;

    return true;
}

bool PosixExtendedParser::parse_root(Vector<ByteCodeValue>& stack, size_t& match_length_minimum)
{
    Vector<ByteCodeValue> bytecode_left;
    size_t match_length_minimum_left { 0 };

    if (match_repetition_symbol())
        return set_error(Error::InvalidRepetitionMarker);

    for (;;) {
        if (!parse_sub_expression(bytecode_left, match_length_minimum_left))
            break;

        if (match(TokenType::Pipe)) {
            consume();

            Vector<ByteCodeValue> bytecode_right;
            size_t match_length_minimum_right { 0 };

            if (!parse_root(bytecode_right, match_length_minimum_right) || bytecode_right.is_empty())
                return set_error(Error::InvalidPattern);

            Vector<ByteCodeValue> new_bytecode;
            insert_bytecode_alternation(new_bytecode, move(bytecode_left), move(bytecode_right));
            bytecode_left = move(new_bytecode);
            match_length_minimum_left = min(match_length_minimum_right, match_length_minimum_left);
        }
    }

    if (bytecode_left.is_empty())
        set_error(Error::EmptySubExpression);

    stack.append(move(bytecode_left));
    match_length_minimum = match_length_minimum_left;
    return !has_error();
}

template class Parser<PosixOptions>;
}
}
