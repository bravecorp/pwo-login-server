/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#ifndef UTILS_RSA_H
#define UTILS_RSA_H

#include <network/networkmessage.h>

class RSA
{
    public:
        RSA() = default;

        // non-copyable
        RSA(const RSA&) = delete;
        RSA& operator=(const RSA&) = delete;

        void loadPEM();
        bool decrypt(NetworkMessage& msg);

    private:
        void decrypt(char* msg);
        CryptoPP::RSA::PrivateKey m_pk;
        CryptoPP::AutoSeededRandomPool m_prng;
};

extern RSA g_RSA;

#endif
