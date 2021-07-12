/*
 * Copyright (c) 2021, timmot <tiwwot@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <FileSystemAccessServer/FileSystemAccessClientEndpoint.h>
#include <FileSystemAccessServer/FileSystemAccessServerEndpoint.h>
#include <LibIPC/ServerConnection.h>

namespace FileSystemAccessClient {

class Client final
    : public IPC::ServerConnection<FileSystemAccessClientEndpoint, FileSystemAccessServerEndpoint>
    , public FileSystemAccessClientEndpoint {
    C_OBJECT(Client)

public:
    virtual void die() override
    {
    }

    void open_file(i32 parent_window_id, Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> handler);

    void save_file(i32 parent_window_id, String const& name, String const ext, Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> handler);

private:
    explicit Client()
        : IPC::ServerConnection<FileSystemAccessClientEndpoint, FileSystemAccessServerEndpoint>(*this, "/tmp/portal/filesystemaccess")
    {
    }

    virtual void handle_prompt_end(i32 error, Optional<IPC::File> const& fd, Optional<String> const& chosen_file) override;

    Function<void(i32, Optional<IPC::File> const&, Optional<String> const&)> m_callback;
};

}
