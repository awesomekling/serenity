#pragma once

#include <AK/Function.h>
#include <AK/String.h>
#include <AK/StringView.h>
#include <AK/Types.h>

#include "Command.h"

#define IAC 0xff

class Parser {
public:
    Function<void(const Command&)> on_command;
    Function<void(const StringView&)> on_data;
    Function<void()> on_error;

    void write(const StringView&);

protected:
    enum State {
        Free,
        ReadCommand,
        ReadSubcommand,
        Error,
    };

    void write(const String& str);

private:
    State m_state { State::Free };
    u8 m_command { 0 };
};
