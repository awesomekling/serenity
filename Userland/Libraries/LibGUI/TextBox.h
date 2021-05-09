/*
 * Copyright (c) 2018-2020, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <LibGUI/TextEditor.h>

namespace GUI {

class TextBox : public TextEditor {
    C_OBJECT(TextBox)
public:
    TextBox();
    virtual ~TextBox() override;

    Function<void()> on_up_pressed;
    Function<void()> on_down_pressed;

    void set_history_enabled(bool enabled) { m_history_enabled = enabled; }
    void add_current_text_to_history();

private:
    virtual void keydown_event(GUI::KeyEvent&) override;

    bool has_no_history() const { return !m_history_enabled || m_history.is_empty(); }
    bool can_go_backwards_in_history() const { return m_history_index > 0; }
    bool can_go_forwards_in_history() const { return m_history_index < static_cast<int>(m_history.size()) - 1; }
    void add_input_to_history(String);

    bool m_history_enabled { false };
    Vector<String> m_history;
    int m_history_index { -1 };
    String m_saved_input;
};

}
