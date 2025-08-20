/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <network/networkmessage.h>

int32_t NetworkMessage::decodeHeader()
{
    int32_t newSize = static_cast<int32_t>(m_buffer[0] | m_buffer[1] << 8);
    m_info.length = (uint16_t)newSize;
    return m_info.length;
}

std::string NetworkMessage::getString(uint16_t stringLen/* = 0*/)
{
    if (stringLen == 0) {
        stringLen = get<uint16_t>();
    }

    if (!canRead(stringLen)) {
        return std::string();
    }

    char* v = reinterpret_cast<char*>(m_buffer) + m_info.position; //does not break strict aliasing
    m_info.position += stringLen;
    return std::string(v, stringLen);
}

void NetworkMessage::addString(const std::string& value)
{
    size_t stringLen = value.length();
    if (!canAdd(stringLen + 2) || stringLen > 8192) {
        return;
    }

    add<uint16_t>(stringLen);
    memcpy(m_buffer + m_info.position, value.c_str(), stringLen);
    m_info.position += stringLen;
    m_info.length += stringLen;
}

void NetworkMessage::addDouble(double value, uint8_t precision/* = 2*/)
{
    addByte(precision);
    add<uint32_t>((value * std::pow(static_cast<float>(10), precision)) + std::numeric_limits<int32_t>::max());
}

void NetworkMessage::addBytes(const char* bytes, size_t size)
{
    if (!canAdd(size) || size > 8192) {
        return;
    }

    memcpy(m_buffer + m_info.position, bytes, size);
    m_info.position += size;
    m_info.length += size;
}

void NetworkMessage::addPaddingBytes(size_t n)
{
    if (!canAdd(n)) {
        return;
    }

    memset(m_buffer + m_info.position, 0x33, n);
    m_info.length += n;
}
