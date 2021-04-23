/*
 * Copyright (c) 2021, kleines Filmröllchen <malu.bertsch@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "Music.h"
#include "Clip.h"
#include "Processor.h"
#include <LibCore/Object.h>

namespace LibDSP {

// A track is also known as a channel and serves as a container for the audio pipeline: clips -> processors -> mixing & output
class Track : public Core::Object {
    C_OBJECT_ABSTRACT(Track)
public:
    Track(NonnullRefPtr<Transport> transport)
    : m_transport(transport)
    {
    }
    virtual ~Track()
    {
    }
    
    virtual bool check_processor_chain_valid() const = 0;
    bool add_processor(NonnullRefPtr<Processor> new_processor);

    NonnullRefPtrVector<Processor> processor_chain() const { return m_processor_chain; }
    const NonnullRefPtr<Transport> transport() const { return m_transport; }

protected:
    bool check_processor_chain_valid_with_initial_type(SignalType initial_type) const;

    NonnullRefPtrVector<Processor> m_processor_chain { };
    const NonnullRefPtr<Transport> m_transport;
};

class NoteTrack final : public Track {
public:
    virtual ~NoteTrack()
    {
    }

    bool check_processor_chain_valid() const override;
    NonnullRefPtrVector<NoteClip> clips() const { return m_clips; }
private:
    NonnullRefPtrVector<NoteClip> m_clips { };
};

class AudioTrack final : public Track {
public:
    virtual ~AudioTrack()
    {
    }

    bool check_processor_chain_valid() const override;
    NonnullRefPtrVector<AudioClip> clips() const { return m_clips; }
private:
    NonnullRefPtrVector<AudioClip> m_clips { };
};

}