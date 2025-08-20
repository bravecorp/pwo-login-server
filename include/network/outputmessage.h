/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef NETWORK_OUTPUTMESSAGE_H
#define NETWORK_OUTPUTMESSAGE_H

#include <network/networkmessage.h>

#include <utils/tools.h>

class OutputMessage : public NetworkMessage
{
    public:
        OutputMessage() = default;

        // non-copyable
        OutputMessage(const OutputMessage&) = delete;
        OutputMessage& operator=(const OutputMessage&) = delete;

        uint8_t* getOutputBuffer() {
            return m_buffer + m_outputBufferStart;
        }

        void writeMessageLength() {
            add_header(m_info.length);
        }

        void addCryptoHeader() {
            add_header(adlerChecksum(m_buffer + m_outputBufferStart, m_info.length));
            writeMessageLength();
        }

    protected:
        template <typename T>
        void add_header(T add) {
            assert(m_outputBufferStart >= sizeof(T));
            m_outputBufferStart -= sizeof(T);
            memcpy(m_buffer + m_outputBufferStart, &add, sizeof(T));
            //added header size to the message size
            m_info.length += sizeof(T);
        }

        MsgSize_t m_outputBufferStart = INITIAL_BUFFER_POSITION;
};

#endif
