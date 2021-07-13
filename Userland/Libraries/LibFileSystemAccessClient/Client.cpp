/*
 * Copyright (c) 2021, timmot <tiwwot@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/AnonymousBuffer.h>
#include <LibCore/StandardPaths.h>
#include <LibFileSystemAccessClient/Client.h>
#include <LibGUI/Application.h>
#include <LibGUI/Window.h>

namespace FileSystemAccessClient {

void Client::open_file(i32 parent_window_id, Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> handler)
{
    m_callback = move(handler);
    m_pending_request = true;

    auto window_server_client_id = GUI::Application::the()->expose_client_id();

    async_prompt_open_file(window_server_client_id, parent_window_id, Core::StandardPaths::home_directory(), Core::OpenMode::ReadOnly);
}

void Client::save_file(i32 parent_window_id, String const& name, String const ext, Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> handler)
{
    m_callback = move(handler);
    m_pending_request = true;

    auto window_server_client_id = GUI::Application::the()->expose_client_id();

    async_prompt_save_file(window_server_client_id, parent_window_id, name.is_null() ? "Untitled" : name, ext.is_null() ? "txt" : ext, Core::StandardPaths::home_directory(), Core::OpenMode::Truncate | Core::OpenMode::WriteOnly);
}

void Client::handle_prompt_end(i32 error, Optional<IPC::File> const& fd, Optional<String> const& chosen_file)
{
    VERIFY(m_pending_request);
    m_pending_request = false;
    m_callback(error, fd, chosen_file);
    m_callback = [](i32, Optional<IPC::File> const&, Optional<String> const&) {
        VERIFY_NOT_REACHED();
    };
}

void Client::die()
{
    if (m_pending_request)
        handle_prompt_end(ECONNRESET, {}, "");
}

}
