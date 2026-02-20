#pragma once

#include <openssl/ssl.h>

enum class TlsHandshakeStep
{
    Done,
    WantRead,
    WantWrite,
    Fail
};

class TlsHandshake
{
public:
    static TlsHandshakeStep drive(SSL* ssl);
};
