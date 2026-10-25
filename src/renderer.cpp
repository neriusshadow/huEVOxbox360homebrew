#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "renderer.h"
#include "config.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace huEVO {

Renderer g_Renderer;

// 8x8 font glyph definition (ASCII 32 to 127)
static const uint8_t s_FontBitmap[96][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // '!'
    {0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00}, // '"'
    {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // '#'
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // '$'
    {0x00,0x63,0x66,0x0C,0x18,0x33,0x63,0x00}, // '%'
    {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // '&'
    {0x30,0x30,0x10,0x20,0x00,0x00,0x00,0x00}, // '''
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // '('
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // '*'
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // '+'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // ','
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // '-'
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // '.'
    {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // '/'
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // '0'
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // '1'
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // '2'
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // '3'
    {0x0C,0x1C,0x34,0x64,0x7E,0x0C,0x0C,0x00}, // '4'
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // '5'
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, // '6'
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, // '7'
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // '8'
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, // '9'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // ':'
    {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // ';'
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // '<'
    {0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00}, // '='
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // '>'
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, // '?'
    {0x3C,0x66,0x6E,0x6E,0x60,0x62,0x3C,0x00}, // '@'
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 'A'
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 'B'
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 'C'
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 'D'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x7E,0x00}, // 'E'
    {0x7E,0x60,0x60,0x7C,0x60,0x60,0x60,0x00}, // 'F'
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3C,0x00}, // 'G'
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 'H'
    {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 'I'
    {0x0E,0x06,0x06,0x06,0x06,0x66,0x3C,0x00}, // 'J'
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 'K'
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 'L'
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 'M'
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 'N'
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 'O'
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 'P'
    {0x3C,0x66,0x66,0x66,0x6E,0x3C,0x0E,0x00}, // 'Q'
    {0x7C,0x66,0x66,0x7C,0x78,0x6C,0x66,0x00}, // 'R'
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 'S'
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 'T'
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 'U'
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 'W'
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 'X'
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 'Y'
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 'Z'
    {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // '['
    {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // '\'
    {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // ']'
    {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, // '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // '_'
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // '`'
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 'a'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 'b'
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 'c'
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 'd'
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 'e'
    {0x1C,0x30,0x78,0x30,0x30,0x30,0x30,0x00}, // 'f'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // 'g'
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 'h'
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 'i'
    {0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x38}, // 'j'
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 'k'
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 'l'
    {0x00,0x00,0x66,0x7F,0x7F,0x6B,0x63,0x00}, // 'm'
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 'n'
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 'o'
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 'p'
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // 'q'
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 'r'
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 's'
    {0x18,0x18,0x7E,0x18,0x18,0x18,0x0C,0x00}, // 't'
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 'u'
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 'v'
    {0x00,0x00,0x63,0x6B,0x7F,0x3E,0x36,0x00}, // 'w'
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 'x'
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x06,0x3C}, // 'y'
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 'z'
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // '{'
    {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // '|'
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // '}'
    {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}  // '~'
};

Renderer::Renderer()
    : m_pD3D(nullptr)
    , m_pDevice(nullptr)
    , m_pFontTexture(nullptr)
    , m_pVertexShader(nullptr)
    , m_pPixelShader(nullptr)
    , m_pVertexDecl(nullptr)
    , m_InFrame(false)
{
}

Renderer::~Renderer() {
    Shutdown();
}

#ifdef _XBOX
static const CHAR* g_strVertexShader =
    " struct VS_IN {                                \n"
    "     float4 Pos   : POSITION;                  \n"
    "     float4 Color : COLOR0;                    \n"
    "     float2 Tex   : TEXCOORD0;                 \n"
    " };                                            \n"
    " struct VS_OUT {                               \n"
    "     float4 ProjPos : POSITION;                \n"
    "     float4 Color   : COLOR0;                  \n"
    " };                                            \n"
    " VS_OUT main( VS_IN In ) {                     \n"
    "     VS_OUT Out;                               \n"
    "     Out.ProjPos.x = (In.Pos.x / 640.0) - 1.0; \n"
    "     Out.ProjPos.y = 1.0 - (In.Pos.y / 360.0); \n"
    "     Out.ProjPos.z = 0.0;                      \n"
    "     Out.ProjPos.w = 1.0;                      \n"
    "     Out.Color = In.Color;                     \n"
    "     return Out;                               \n"
    " }                                             \n";

static const CHAR* g_strPixelShader =
    " struct PS_IN {                                \n"
    "     float4 Color : COLOR0;                    \n"
    " };                                            \n"
    " float4 main( PS_IN In ) : COLOR0 {            \n"
    "     return In.Color;                          \n"
    " }                                             \n";
#endif

bool Renderer::Initialize() {
#ifdef _XBOX
    // Set up presentation parameters matching official Xbox 360 standards
    D3DPRESENT_PARAMETERS d3dpp;
    ZeroMemory(&d3dpp, sizeof(d3dpp));
    d3dpp.BackBufferWidth = SCREEN_WIDTH;
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;
    d3dpp.BackBufferFormat = (D3DFORMAT)MAKESRGBFMT(D3DFMT_A8R8G8B8);
    d3dpp.FrontBufferFormat = (D3DFORMAT)MAKESRGBFMT(D3DFMT_LE_X8R8G8B8);
    d3dpp.MultiSampleType = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality = 0;
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE; // 60 FPS VSync

    HRESULT hr = Direct3D_CreateDevice(
        0,
        D3DDEVTYPE_HAL,
        NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp,
        &m_pDevice
    );
    if (FAILED(hr) || !m_pDevice) return false;

    // Compile and create Vertex Shader
    ID3DXBuffer* pVSBuffer = NULL;
    ID3DXBuffer* pErrorMsg = NULL;
    hr = D3DXCompileShader(
        g_strVertexShader,
        (UINT)strlen(g_strVertexShader),
        NULL, NULL,
        "main", "vs_2_0", 0,
        &pVSBuffer, &pErrorMsg, NULL
    );
    if (SUCCEEDED(hr) && pVSBuffer) {
        m_pDevice->CreateVertexShader((const DWORD*)pVSBuffer->GetBufferPointer(), &m_pVertexShader);
        pVSBuffer->Release();
    }

    // Compile and create Pixel Shader
    ID3DXBuffer* pPSBuffer = NULL;
    hr = D3DXCompileShader(
        g_strPixelShader,
        (UINT)strlen(g_strPixelShader),
        NULL, NULL,
        "main", "ps_2_0", 0,
        &pPSBuffer, &pErrorMsg, NULL
    );
    if (SUCCEEDED(hr) && pPSBuffer) {
        m_pDevice->CreatePixelShader((const DWORD*)pPSBuffer->GetBufferPointer(), &m_pPixelShader);
        pPSBuffer->Release();
    }

    // Create Vertex Declaration matching Vertex2D
    static const D3DVERTEXELEMENT9 s_VertexElements[] = {
        { 0,  0, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        { 0, 20, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    m_pDevice->CreateVertexDeclaration(s_VertexElements, &m_pVertexDecl);

    // Set 2D render states
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
#endif
    return true;
}

void Renderer::Shutdown() {
#ifdef _XBOX
    if (m_pVertexDecl) {
        m_pVertexDecl->Release();
        m_pVertexDecl = nullptr;
    }
    if (m_pPixelShader) {
        m_pPixelShader->Release();
        m_pPixelShader = nullptr;
    }
    if (m_pVertexShader) {
        m_pVertexShader->Release();
        m_pVertexShader = nullptr;
    }
    if (m_pFontTexture) {
        m_pFontTexture->Release();
        m_pFontTexture = nullptr;
    }
    if (m_pDevice) {
        m_pDevice->Release();
        m_pDevice = nullptr;
    }
    if (m_pD3D) {
        m_pD3D->Release();
        m_pD3D = nullptr;
    }
#endif
}

void Renderer::BeginFrame() {
    m_InFrame = true;
    m_VertexBuffer.clear();
#ifdef _XBOX
    if (m_pDevice) {
        m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, COLOR_BG, 1.0f, 0);
        m_pDevice->BeginScene();
        if (m_pVertexShader) m_pDevice->SetVertexShader(m_pVertexShader);
        if (m_pPixelShader)  m_pDevice->SetPixelShader(m_pPixelShader);
        if (m_pVertexDecl)   m_pDevice->SetVertexDeclaration(m_pVertexDecl);
    }
#endif
}

void Renderer::EndFrame() {
    FlushBatch();
#ifdef _XBOX
    if (m_pDevice) {
        m_pDevice->EndScene();
        m_pDevice->Present(NULL, NULL, NULL, NULL);
    }
#endif
    m_InFrame = false;
}

void Renderer::FlushBatch() {
    if (m_VertexBuffer.empty()) return;

#ifdef _XBOX
    if (m_pDevice) {
        if (m_pVertexShader) m_pDevice->SetVertexShader(m_pVertexShader);
        if (m_pPixelShader)  m_pDevice->SetPixelShader(m_pPixelShader);
        if (m_pVertexDecl)   m_pDevice->SetVertexDeclaration(m_pVertexDecl);

        m_pDevice->DrawPrimitiveUP(
            D3DPT_TRIANGLELIST,
            m_VertexBuffer.size() / 3,
            m_VertexBuffer.data(),
            sizeof(Vertex2D)
        );
    }
#endif
    m_VertexBuffer.clear();
}

void Renderer::DrawRect(float x, float y, float w, float h, uint32_t color) {
    Vertex2D v1 = { x,     y,     0.0f, 1.0f, color, 0.0f, 0.0f };
    Vertex2D v2 = { x + w, y,     0.0f, 1.0f, color, 0.0f, 0.0f };
    Vertex2D v3 = { x,     y + h, 0.0f, 1.0f, color, 0.0f, 0.0f };
    Vertex2D v4 = { x + w, y + h, 0.0f, 1.0f, color, 0.0f, 0.0f };

    m_VertexBuffer.push_back(v1);
    m_VertexBuffer.push_back(v2);
    m_VertexBuffer.push_back(v3);

    m_VertexBuffer.push_back(v2);
    m_VertexBuffer.push_back(v4);
    m_VertexBuffer.push_back(v3);
}

void Renderer::DrawGradientV(float x, float y, float w, float h, uint32_t topColor, uint32_t bottomColor) {
    Vertex2D v1 = { x,     y,     0.0f, 1.0f, topColor,    0.0f, 0.0f };
    Vertex2D v2 = { x + w, y,     0.0f, 1.0f, topColor,    0.0f, 0.0f };
    Vertex2D v3 = { x,     y + h, 0.0f, 1.0f, bottomColor, 0.0f, 0.0f };
    Vertex2D v4 = { x + w, y + h, 0.0f, 1.0f, bottomColor, 0.0f, 0.0f };

    m_VertexBuffer.push_back(v1);
    m_VertexBuffer.push_back(v2);
    m_VertexBuffer.push_back(v3);

    m_VertexBuffer.push_back(v2);
    m_VertexBuffer.push_back(v4);
    m_VertexBuffer.push_back(v3);
}

void Renderer::DrawLine(float x1, float y1, float x2, float y2, uint32_t color, float thickness) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx * dx + dy * dy);
    if (len <= 0.0001f) return;

    float nx = (-dy / len) * (thickness * 0.5f);
    float ny = (dx / len) * (thickness * 0.5f);

    Vertex2D v1 = { x1 + nx, y1 + ny, 0.0f, 1.0f, color, 0.0f, 0.0f };
    Vertex2D v2 = { x2 + nx, y2 + ny, 0.0f, 1.0f, color, 0.0f, 0.0f };
    Vertex2D v3 = { x1 - nx, y1 - ny, 0.0f, 1.0f, color, 0.0f, 0.0f };
    Vertex2D v4 = { x2 - nx, y2 - ny, 0.0f, 1.0f, color, 0.0f, 0.0f };

    m_VertexBuffer.push_back(v1);
    m_VertexBuffer.push_back(v2);
    m_VertexBuffer.push_back(v3);

    m_VertexBuffer.push_back(v2);
    m_VertexBuffer.push_back(v4);
    m_VertexBuffer.push_back(v3);
}

void Renderer::DrawRoundedRect(float x, float y, float w, float h, float radius, uint32_t fillColor, uint32_t borderColor, float borderWidth) {
    // Center & side quads
    DrawRect(x + radius, y, w - 2 * radius, h, fillColor);
    DrawRect(x, y + radius, radius, h - 2 * radius, fillColor);
    DrawRect(x + w - radius, y + radius, radius, h - 2 * radius, fillColor);

    // Subtle border outline if requested
    if (borderColor != 0 && borderWidth > 0.0f) {
        DrawLine(x + radius, y, x + w - radius, y, borderColor, borderWidth);
        DrawLine(x + radius, y + h, x + w - radius, y + h, borderColor, borderWidth);
        DrawLine(x, y + radius, x, y + h - radius, borderColor, borderWidth);
        DrawLine(x + w, y + radius, x + w, y + h - radius, borderColor, borderWidth);
    }
}

float Renderer::MeasureTextWidth(const std::string& text, float scale) {
    float charW = 9.0f * scale;
    return text.length() * charW;
}

float Renderer::GetFontHeight(float scale) {
    return 16.0f * scale;
}

void Renderer::DrawText(const std::string& text, float x, float y, uint32_t color, float scale, bool shadow) {
    if (shadow) {
        uint32_t shadowColor = 0xBB000000;
        DrawText(text, x + 1.5f * scale, y + 1.5f * scale, shadowColor, scale, false);
    }

    float curX = x;
    float curY = y;
    float pixelSize = scale;

    for (size_t i = 0; i < text.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c == '\n') {
            curX = x;
            curY += 16.0f * scale;
            continue;
        }

        // Support ASCII
        int glyphIndex = c - 32;
        if (glyphIndex >= 0 && glyphIndex < 96) {
            for (int row = 0; row < 8; ++row) {
                uint8_t rowBits = s_FontBitmap[glyphIndex][row];
                for (int col = 0; col < 8; ++col) {
                    if (rowBits & (0x80 >> col)) {
                        DrawRect(
                            curX + col * pixelSize,
                            curY + row * pixelSize,
                            pixelSize,
                            pixelSize,
                            color
                        );
                    }
                }
            }
        }
        curX += 9.0f * scale;
    }
}

void Renderer::DrawButtonPrompt(const std::string& buttonName, const std::string& label, float x, float y, uint32_t buttonColor) {
    float badgeR = 11.0f;
    DrawRoundedRect(x, y - 2, badgeR * 2, badgeR * 2, 4.0f, buttonColor);
    DrawText(buttonName, x + 6.0f, y + 2.0f, 0xFFFFFFFF, 1.0f, false);
    DrawText(label, x + badgeR * 2 + 8.0f, y + 2.0f, COLOR_TEXT_PRIMARY, 1.1f, true);
}

void Renderer::DrawBadge(const std::string& text, float x, float y, uint32_t bgColor, uint32_t textColor) {
    float w = MeasureTextWidth(text, 0.9f) + 12.0f;
    float h = 18.0f;
    DrawRoundedRect(x, y, w, h, 4.0f, bgColor);
    DrawText(text, x + 6.0f, y + 2.0f, textColor, 0.9f, false);
}

void Renderer::DrawProgressBar(float x, float y, float w, float h, float progress, uint32_t fillColor, uint32_t bgColor) {
    progress = (std::max)(0.0f, (std::min)(1.0f, progress));
    DrawRoundedRect(x, y, w, h, 3.0f, bgColor);
    if (progress > 0.01f) {
        DrawRoundedRect(x, y, w * progress, h, 3.0f, fillColor);
    }
}

} // namespace huEVO
