/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef NETWORK_NETWORKMESSAGE_H
#define NETWORK_NETWORKMESSAGE_H

static constexpr int32_t NETWORKMESSAGE_MAXSIZE = 24590;

class NetworkMessage
{
    public:
        using MsgSize_t = uint16_t;

        // Headers:
        // 2 bytes for unencrypted message size
        // 4 bytes for checksum
        // 2 bytes for encrypted message size
        static constexpr MsgSize_t INITIAL_BUFFER_POSITION = 8;
        enum { HEADER_LENGTH = 2 };
        enum { CHECKSUM_LENGTH = 4 };
        enum { XTEA_MULTIPLE = 8 };
        enum { MAX_BODY_LENGTH = NETWORKMESSAGE_MAXSIZE - HEADER_LENGTH - CHECKSUM_LENGTH - XTEA_MULTIPLE };
        enum { MAX_PROTOCOL_BODY_LENGTH = MAX_BODY_LENGTH - 10 };

        NetworkMessage() = default;

        void reset() {
            m_info = {};
        }

        // simply read functions for incoming message
        uint8_t getByte() {
            if (!canRead(1)) {
                return 0;
            }

            return m_buffer[m_info.position++];
        }

        uint8_t getPreviousByte() {
            return m_buffer[--m_info.position];
        }

        template<typename T>
        T get() {
            if (!canRead(sizeof(T))) {
                return 0;
            }

            T v;
            memcpy(&v, m_buffer + m_info.position, sizeof(T));
            m_info.position += sizeof(T);
            return v;
        }

        std::string getString(uint16_t stringLen = 0);

        // skips count unknown/unused bytes in an incoming message
        void skipBytes(int16_t count) {
            m_info.position += count;
        }

        // simply write functions for outgoing message
        void addByte(uint8_t value) {
            if (!canAdd(1)) {
                return;
            }

            m_buffer[m_info.position++] = value;
            m_info.length++;
        }

        template<typename T>
        void add(T value) {
            if (!canAdd(sizeof(T))) {
                return;
            }

            memcpy(m_buffer + m_info.position, &value, sizeof(T));
            m_info.position += sizeof(T);
            m_info.length += sizeof(T);
        }

        void addBytes(const char* bytes, size_t size);
        void addPaddingBytes(size_t n);

        void addString(const std::string& value);

        void addDouble(double value, uint8_t precision = 2);

        MsgSize_t getLength() const {
            return m_info.length;
        }

        void setLength(MsgSize_t newLength) {
            m_info.length = newLength;
        }

        MsgSize_t getBufferPosition() const {
            return m_info.position;
        }

        uint16_t getLengthHeader() const {
            return static_cast<uint16_t>(m_buffer[0] | m_buffer[1] << 8);
        }

        int32_t decodeHeader();

        bool isOverrun() const {
            return m_info.overrun;
        }

        uint8_t* getBuffer() {
            return m_buffer;
        }

        const uint8_t* getBuffer() const {
            return m_buffer;
        }

        uint8_t* getBodyBuffer() {
            m_info.position = 2;
            return m_buffer + HEADER_LENGTH;
        }

        static const uint8_t headerLength = 2;

    protected:
        bool canAdd(size_t size) const {
            return (size + m_info.position) < MAX_BODY_LENGTH;
        }

        bool canRead(int32_t size) {
            if ((m_info.position + size) > (m_info.length + 8) || size >= (NETWORKMESSAGE_MAXSIZE - m_info.position)) {
                m_info.overrun = true;
                return false;
            }
            return true;
        }

        struct NetworkMessageInfo {
            MsgSize_t length = 0;
            MsgSize_t position = INITIAL_BUFFER_POSITION;
            bool overrun = false;
        };

        NetworkMessageInfo m_info;
        uint8_t m_buffer[NETWORKMESSAGE_MAXSIZE];
};

#endif
