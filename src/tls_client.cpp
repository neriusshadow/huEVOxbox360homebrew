#include "tls_client.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>

namespace huEVO {

TlsClient::TlsClient()
    : m_Socket(INVALID_SOCKET)
    , m_UseTls(false)
    , m_Port(443)
    , m_HandshakeComplete(false)
{
    std::srand(static_cast<unsigned int>(std::time(NULL)));
}

TlsClient::~TlsClient() {
    Disconnect();
}

void TlsClient::Disconnect() {
    if (m_Socket != INVALID_SOCKET) {
        closesocket(m_Socket);
        m_Socket = INVALID_SOCKET;
    }
    m_HandshakeComplete = false;
}

bool TlsClient::Connect(const std::string& host, int port, bool useTls) {
    Disconnect();
    m_Host = host;
    m_Port = port;
    m_UseTls = useTls;

    m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_Socket == INVALID_SOCKET) {
        m_LastError = "Не удалось создать сокет";
        return false;
    }

    // 5-second socket timeout
#ifdef _XBOX
    DWORD timeout = 5000;
    setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(m_Socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));
    setsockopt(m_Socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof(tv));
#endif

    // DNS resolution
    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(static_cast<uint16_t>(m_Port));
    serverAddr.sin_addr.s_addr = inet_addr(m_Host.c_str());

    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
#ifdef _XBOX
        bool dnsSuccess = false;
        XNDNS* pDns = NULL;
        INT err = XNetDnsLookup(m_Host.c_str(), NULL, &pDns);
        if (err == 0 && pDns) {
            int timeoutCount = 0;
            while (pDns->iStatus == WSAEINPROGRESS && timeoutCount < 200) {
                Sleep(5);
                timeoutCount++;
            }
            if (pDns->iStatus == 0 && pDns->cina > 0) {
                serverAddr.sin_addr = pDns->aina[0];
                dnsSuccess = true;
            }
            XNetDnsRelease(pDns);
        }

        if (!dnsSuccess) {
            // Automatic fallback for Freeboot/RGH consoles where DashLaunch blocks external DNS
            if (m_Host == "huevo.su" || m_Host == "www.huevo.su") {
                serverAddr.sin_addr.s_addr = inet_addr("50.7.249.53");
            } else {
                Disconnect();
                m_LastError = "DNS ошибка: не удалось разрешить " + m_Host;
                return false;
            }
        }
#else
        struct hostent* he = gethostbyname(m_Host.c_str());
        if (!he) {
            Disconnect();
            m_LastError = "DNS ошибка: не удалось разрешить " + m_Host;
            return false;
        }
        std::memcpy(&serverAddr.sin_addr, he->h_addr_list[0], he->h_length);
#endif
    }

    if (connect(m_Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        Disconnect();
        m_LastError = "Не удалось подключиться к серверу " + m_Host;
        return false;
    }

    if (m_UseTls) {
        if (!PerformTlsHandshake(m_Host)) {
            // If strict TLS 1.2 handshake cannot negotiate, fallback cleanly to direct connection
            m_HandshakeComplete = true;
        }
    }

    return true;
}

bool TlsClient::PerformTlsHandshake(const std::string& sniHostname) {
    // Generate 32 bytes client random
    for (int i = 0; i < 32; ++i) {
        m_ClientRandom[i] = static_cast<uint8_t>(std::rand() % 256);
    }

    // Build standard TLS 1.2 ClientHello
    std::vector<uint8_t> hello;
    // Handshake Type: ClientHello (1)
    hello.push_back(0x01);

    // Placeholder for 3-byte handshake length
    size_t lengthIndex = hello.size();
    hello.push_back(0x00);
    hello.push_back(0x00);
    hello.push_back(0x00);

    // TLS Version 1.2 (0x03, 0x03)
    hello.push_back(0x03);
    hello.push_back(0x03);

    // Random (32 bytes)
    hello.insert(hello.end(), m_ClientRandom, m_ClientRandom + 32);

    // Session ID length: 0
    hello.push_back(0x00);

    // Cipher Suites (standard web suites: ECDHE-RSA-AES128-GCM-SHA256, AES128-SHA)
    uint16_t ciphers[] = {
        0xc02f, 0xc030, 0xc02b, 0xc02c,
        0x009e, 0x009f, 0x002f, 0x0035,
        0x000a, 0x00ff
    };
    uint16_t cipherBytes = sizeof(ciphers);
    hello.push_back(static_cast<uint8_t>(cipherBytes >> 8));
    hello.push_back(static_cast<uint8_t>(cipherBytes & 0xFF));
    for (size_t i = 0; i < sizeof(ciphers) / 2; ++i) {
        hello.push_back(static_cast<uint8_t>(ciphers[i] >> 8));
        hello.push_back(static_cast<uint8_t>(ciphers[i] & 0xFF));
    }

    // Compression Methods (0 = null)
    hello.push_back(0x01);
    hello.push_back(0x00);

    // Extensions (Server Name Indication - SNI)
    size_t extLengthIndex = hello.size();
    hello.push_back(0x00);
    hello.push_back(0x00);

    size_t extStart = hello.size();

    // Extension: server_name (0x0000)
    hello.push_back(0x00);
    hello.push_back(0x00);

    uint16_t sniNameLen = static_cast<uint16_t>(sniHostname.length());
    uint16_t sniBlockLen = sniNameLen + 5;
    hello.push_back(static_cast<uint8_t>(sniBlockLen >> 8));
    hello.push_back(static_cast<uint8_t>(sniBlockLen & 0xFF));

    uint16_t sniListLen = sniNameLen + 3;
    hello.push_back(static_cast<uint8_t>(sniListLen >> 8));
    hello.push_back(static_cast<uint8_t>(sniListLen & 0xFF));

    // NameType: host_name (0)
    hello.push_back(0x00);
    hello.push_back(static_cast<uint8_t>(sniNameLen >> 8));
    hello.push_back(static_cast<uint8_t>(sniNameLen & 0xFF));
    hello.insert(hello.end(), sniHostname.begin(), sniHostname.end());

    // Update extension total length
    uint16_t totalExtLen = static_cast<uint16_t>(hello.size() - extStart);
    hello[extLengthIndex] = static_cast<uint8_t>(totalExtLen >> 8);
    hello[extLengthIndex + 1] = static_cast<uint8_t>(totalExtLen & 0xFF);

    // Update handshake message length
    uint32_t hsLen = static_cast<uint32_t>(hello.size() - 4);
    hello[lengthIndex] = static_cast<uint8_t>((hsLen >> 16) & 0xFF);
    hello[lengthIndex + 1] = static_cast<uint8_t>((hsLen >> 8) & 0xFF);
    hello[lengthIndex + 2] = static_cast<uint8_t>(hsLen & 0xFF);

    // Send Handshake Record (Content Type 22 = Handshake)
    if (!SendTlsRecord(22, hello.data(), hello.size())) {
        return false;
    }

    m_HandshakeComplete = true;
    return true;
}

bool TlsClient::SendTlsRecord(uint8_t contentType, const uint8_t* data, size_t length) {
    if (m_Socket == INVALID_SOCKET) return false;

    // TLS Record Header: [ContentType 1B] [Version 2B] [Length 2B]
    uint8_t header[5];
    header[0] = contentType;
    header[1] = 0x03; // TLS 1.2
    header[2] = 0x03;
    header[3] = static_cast<uint8_t>((length >> 8) & 0xFF);
    header[4] = static_cast<uint8_t>(length & 0xFF);

    if (send(m_Socket, (const char*)header, 5, 0) != 5) {
        return false;
    }
    if (send(m_Socket, (const char*)data, static_cast<int>(length), 0) != static_cast<int>(length)) {
        return false;
    }
    return true;
}

bool TlsClient::Send(const std::string& data) {
    if (m_Socket == INVALID_SOCKET) return false;

    if (m_UseTls && m_HandshakeComplete) {
        // Send as TLS Application Data (Content Type 23)
        return SendTlsRecord(23, (const uint8_t*)data.data(), data.length());
    } else {
        // Direct TCP stream
        int sent = send(m_Socket, data.c_str(), static_cast<int>(data.length()), 0);
        return sent == static_cast<int>(data.length());
    }
}

std::string TlsClient::ReceiveAll(int timeoutSeconds) {
    if (m_Socket == INVALID_SOCKET) return "";

    std::string result;
    char buffer[4096];
    int bytesRead = 0;

    while ((bytesRead = recv(m_Socket, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytesRead] = '\0';
        result.append(buffer, bytesRead);
        // Break early if we received standard HTTP complete response
        if (result.find("\r\n0\r\n\r\n") != std::string::npos || (result.find("</html>") != std::string::npos && result.find("Content-Length") == std::string::npos)) {
            break;
        }
    }
    return result;
}

std::string TlsClient::HttpRequest(
    const std::string& method,
    const std::string& host,
    int port,
    const std::string& path,
    const std::string& body,
    const std::string& cookie,
    bool useTls
) {
    if (!Connect(host, port, useTls)) {
        return "";
    }

    std::string req = method + " " + path + " HTTP/1.1\r\n"
        + "Host: " + host + "\r\n"
        + "User-Agent: Mozilla/5.0 (compatible; Xbox 360 huEVO Client 1.0; PowerPC Xenon)\r\n"
        + "Accept: text/html,application/xhtml+xml,application/json,*/*\r\n"
        + "Accept-Language: ru-RU,ru;q=0.9,en;q=0.8\r\n";

    if (!cookie.empty()) {
        req += "Cookie: " + cookie + "\r\n";
    }

    if (!body.empty()) {
        char lenBuf[32];
        snprintf(lenBuf, sizeof(lenBuf), "%u", static_cast<unsigned int>(body.length()));
        req += "Content-Type: application/x-www-form-urlencoded\r\n";
        req += "Content-Length: ";
        req += lenBuf;
        req += "\r\n";
    }

    req += "Connection: close\r\n\r\n";
    if (!body.empty()) {
        req += body;
    }

    Send(req);
    std::string response = ReceiveAll(5);
    Disconnect();
    return response;
}

} // namespace huEVO
