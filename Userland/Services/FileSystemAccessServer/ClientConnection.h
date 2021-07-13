/*
 * Copyright (c) 2021, timmot <tiwwot@protonmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <AK/HashMap.h>
#include <FileSystemAccessServer/FileSystemAccessClientEndpoint.h>
#include <FileSystemAccessServer/FileSystemAccessServerEndpoint.h>
#include <LibCore/Forward.h>
#include <LibIPC/ClientConnection.h>

namespace FileSystemAccessServer {

class ClientConnection final
    : public IPC::ClientConnection<FileSystemAccessClientEndpoint, FileSystemAccessServerEndpoint> {
    C_OBJECT(ClientConnection);

public:
    explicit ClientConnection(NonnullRefPtr<Core::LocalSocket>, int client_id);
    ~ClientConnection() override;

    virtual void die() override;

private:
    virtual Messages::FileSystemAccessServer::RequestFileResponse request_file(String const&, Core::OpenMode const&) override;
    virtual void prompt_open_file(i32, i32, String const&, Core::OpenMode const&) override;
    virtual void prompt_save_file(i32, i32, String const&, String const&, String const&, Core::OpenMode const&) override;

    void prompt_helper(Optional<String> const&, Core::OpenMode const&);

    HashMap<String, Core::OpenMode> m_approved_files;
};

}
