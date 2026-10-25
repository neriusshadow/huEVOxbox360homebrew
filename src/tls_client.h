#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#include <vector>

#ifdef _XBOX
#include <xtl.h>
#include <winsockx.h>
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned int uint32_t;
#else
#include <cstdint>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#define closesocket close
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace huEVO {

/**
 * Direct HTTPS/TLS Stream Transport for Xbox 360.
 * Allows the console to connect directly to https://huevo.su on port 443
 * over wired Ethernet without any external PC or bridge!
 */
class TlsClient {
public:
    TlsClient();
    ~TlsClient();

    // Connect directly to host on port 443 (HTTPS) or 80 (HTTP)
    bool Connect(const std::string& host, int port = 443, bool useTls = true);
    void Disconnect();

    // Send and receive over direct TLS stream
    bool Send(const std::string& data);
    std::string ReceiveAll(int timeoutSeconds = 6);

    // Direct HTTP/HTTPS transaction helper
    std::string HttpRequest(
        const std::string& method,
        const std::string& host,
        int port,
        const std::string& path,
        const std::string& body = "",
        const std::string& cookie = "",
        bool useTls = true
    );

    bool IsConnected() const { return m_Socket != INVALID_SOCKET; }
    std::string GetLastError() const { return m_LastError; }

private:
    bool PerformTlsHandshake(const std::string& sniHostname);
    bool SendTlsRecord(uint8_t contentType, const uint8_t* data, size_t length);

    SOCKET m_Socket;
    bool m_UseTls;
    std::string m_Host;
    int m_Port;
    std::string m_LastError;

    // TLS 1.2 session state
    bool m_HandshakeComplete;
    uint8_t m_ClientRandom[32];
    uint8_t m_ServerRandom[32];
};

} // namespace huEVO
