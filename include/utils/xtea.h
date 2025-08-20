/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef UTILS_XTEA_H
#define UTILS_XTEA_H

#include <network/outputmessage.h>

class XTEA {
    public:
        XTEA() = default;
        ~XTEA() = default;

        // non-copyable
        XTEA(const XTEA&) = delete;
        XTEA& operator=(const XTEA&) = delete;

        void encrypt(uint32_t* key, OutputMessage& msg) const;
        bool decrypt(uint32_t* key, NetworkMessage& msg) const;

    private:
        uint32_t m_delta = 0x61C88647;
};

extern XTEA g_XTEA;

#endif
