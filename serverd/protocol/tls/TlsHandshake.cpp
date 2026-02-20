#include "protocol/tls/TlsHandshake.h"

TlsHandshakeStep TlsHandshake::drive(SSL* ssl)
{
    int ret = SSL_accept(ssl);
    if (ret == 1)
    {
        return TlsHandshakeStep::Done;
    }

    int err = SSL_get_error(ssl, ret);

    if (err == SSL_ERROR_WANT_READ)
    {
        return TlsHandshakeStep::WantRead;
    }
    if (err == SSL_ERROR_WANT_WRITE)
    {
        return TlsHandshakeStep::WantWrite;
    }

    return TlsHandshakeStep::Fail;
}
