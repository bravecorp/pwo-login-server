/**
 * Copyright (c) 2025 PWO Team. All rights reserved.
 * This code is confidential and intended solely for internal use by authorized personnel.
 * Any unauthorized reproduction, distribution, or disclosure — including publication or sharing outside the company —
 * is strictly prohibited and may result in legal action.
 */

#include "includes.h"

#include <utils/rsa.h>

RSA g_RSA;

void RSA::loadPEM()
{
    static const std::string header = "-----BEGIN RSA PRIVATE KEY-----";
    static const std::string footer = "-----END RSA PRIVATE KEY-----";

    std::ifstream file{"key.pem"};

    if (!file.is_open()) {
        throw std::runtime_error("Missing file.");
     }

    std::ostringstream oss;
    for (std::string line; std::getline(file, line); oss << line);
    std::string key = oss.str();

    if (key.substr(0, header.size()) != header) {
        throw std::runtime_error("Missing RSA private key header.");
    }

    if (key.substr(key.size() - footer.size(), footer.size()) != footer) {
        throw std::runtime_error("Missing RSA private key footer.");
    }

    key = key.substr(header.size(), key.size() - footer.size());

    CryptoPP::ByteQueue queue;
    CryptoPP::Base64Decoder decoder;
    decoder.Attach(new CryptoPP::Redirector(queue));
    decoder.Put(reinterpret_cast<const uint8_t*>(key.c_str()), key.size());
    decoder.MessageEnd();

    m_pk.BERDecodePrivateKey(queue, false, queue.MaxRetrievable());

    if (!m_pk.Validate(m_prng, 3)) {
        throw std::runtime_error("RSA private key is not valid.");
    }
}

bool RSA::decrypt(NetworkMessage& msg)
{
    if ((msg.getLength() - msg.getBufferPosition()) < 128) {
        return false;
    }

    decrypt(reinterpret_cast<char*>(msg.getBuffer()) + msg.getBufferPosition()); //does not break strict aliasing
    return msg.getByte() == 0;
}

void RSA::decrypt(char* msg)
{
    CryptoPP::Integer m{reinterpret_cast<uint8_t*>(msg), 128};
    auto c = m_pk.CalculateInverse(m_prng, m);
    c.Encode(reinterpret_cast<uint8_t*>(msg), 128);
}
