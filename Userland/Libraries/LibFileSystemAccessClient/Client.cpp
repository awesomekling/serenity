/*
 * Copyright (c) 2021, timmot <tiwwot@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <LibCore/AnonymousBuffer.h>
#include <LibCore/StandardPaths.h>
#include <LibFileSystemAccessClient/Client.h>

namespace FileSystemAccessClient {

void Client::open_file(Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> new_callback)
{
    m_callback = move(new_callback);

    async_prompt_open_file(Core::StandardPaths::home_directory(), Core::OpenMode::ReadOnly);
}

void Client::save_file(String const& name, String const ext, Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> new_callback)
{
    m_callback = move(new_callback);

    async_prompt_save_file(name.is_null() ? "Untitled" : name, ext.is_null() ? "txt" : ext, Core::StandardPaths::home_directory(), Core::OpenMode::Truncate | Core::OpenMode::WriteOnly);
}

void Client::handle_prompt_end(i32 error, Optional<IPC::File> const& fd, Optional<String> const& chosen_file)
{
    m_callback(error, fd, chosen_file);
    m_callback = [](i32, Optional<IPC::File> const&, Optional<String> const&) {
        VERIFY_NOT_REACHED();
    };
}

}
