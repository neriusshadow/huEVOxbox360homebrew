#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "renderer.h"
#include "font.h"
#include "config.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace huEVO {

Renderer g_Renderer;

Renderer::Renderer()
    : m_pD3D(nullptr)
    , m_pDevice(nullptr)
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
    m_pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!m_pD3D) return false;

    D3DPRESENT_PARAMETERS d3dpp;
    std::memset(&d3dpp, 0, sizeof(d3dpp));
    d3dpp.BackBufferWidth = SCREEN_WIDTH;
    d3dpp.BackBufferHeight = SCREEN_HEIGHT;
    d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
    d3dpp.BackBufferCount = 1;
    d3dpp.EnableAutoDepthStencil = FALSE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

    HRESULT hr = m_pD3D->CreateDevice(
        0,
        D3DDEVTYPE_HAL,
        NULL,
        D3DCREATE_HARDWARE_VERTEXPROCESSING,
        &d3dpp,
        &m_pDevice
    );
    if (FAILED(hr) || !m_pDevice) return false;

    // Compile Vertex Shader
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

    // Compile Pixel Shader
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

    // Vertex declaration
    static const D3DVERTEXELEMENT9 s_VertexElements[] = {
        { 0,  0, D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 16, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        { 0, 20, D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
        D3DDECL_END()
    };
    m_pDevice->CreateVertexDeclaration(s_VertexElements, &m_pVertexDecl);

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
        m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, COLOR_BG, 1.0f, 0);
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
    if (len < 0.001f) return;

    float nx = -dy / len * (thickness * 0.5f);
    float ny =  dx / len * (thickness * 0.5f);

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
    DrawRect(x + radius, y, w - 2 * radius, h, fillColor);
    DrawRect(x, y + radius, radius, h - 2 * radius, fillColor);
    DrawRect(x + w - radius, y + radius, radius, h - 2 * radius, fillColor);

    if (borderColor != 0 && borderWidth > 0.0f) {
        DrawLine(x + radius, y, x + w - radius, y, borderColor, borderWidth);
        DrawLine(x + radius, y + h, x + w - radius, y + h, borderColor, borderWidth);
        DrawLine(x, y + radius, x, y + h - radius, borderColor, borderWidth);
        DrawLine(x + w, y + radius, x + w, y + h - radius, borderColor, borderWidth);
    }
}

void Renderer::DrawDivider(float x, float y, float w, uint32_t color) {
    DrawRect(x, y, w, 1.0f, color);
}

float Renderer::MeasureTextWidth(const std::string& text, float scale) {
    float charW = 8.5f * scale;
    const char* p = text.c_str();
    size_t remaining = text.length();
    int glyphCount = 0;

    while (remaining > 0) {
        uint32_t cp = DecodeUtf8Codepoint(p, remaining);
        if (cp == '\n') break;
        glyphCount++;
    }
    return glyphCount * charW;
}

float Renderer::GetFontHeight(float scale) {
    return 16.0f * scale;
}

void Renderer::DrawText(const std::string& text, float x, float y, uint32_t color, float scale, bool shadow) {
    if (shadow) {
        uint32_t shadowColor = 0xCC050507;
        DrawText(text, x + 1.5f * scale, y + 1.5f * scale, shadowColor, scale, false);
    }

    float curX = x;
    float curY = y;
    float pixelSize = scale;
    float charAdvance = 8.5f * scale;

    const char* p = text.c_str();
    size_t remaining = text.length();

    while (remaining > 0) {
        uint32_t cp = DecodeUtf8Codepoint(p, remaining);
        if (cp == '\n') {
            curX = x;
            curY += 16.0f * scale;
            continue;
        }

        const uint8_t* glyph = GetGlyphBitmap(cp);
        for (int row = 0; row < 8; ++row) {
            uint8_t rowBits = glyph[row];
            if (!rowBits) continue;

            int col = 0;
            while (col < 8) {
                if (rowBits & (0x80 >> col)) {
                    int runLen = 1;
                    while (col + runLen < 8 && (rowBits & (0x80 >> (col + runLen)))) {
                        runLen++;
                    }
                    DrawRect(
                        curX + col * pixelSize,
                        curY + row * pixelSize,
                        runLen * pixelSize,
                        pixelSize,
                        color
                    );
                    col += runLen;
                } else {
                    col++;
                }
            }
        }
        curX += charAdvance;
    }
}

void Renderer::DrawTextClipped(const std::string& text, float x, float y, float maxWidth, uint32_t color, float scale, bool shadow) {
    float curW = MeasureTextWidth(text, scale);
    if (curW <= maxWidth) {
        DrawText(text, x, y, color, scale, shadow);
        return;
    }

    // Clip with ellipsis
    std::string clipped;
    const char* p = text.c_str();
    size_t remaining = text.length();
    float dotW = MeasureTextWidth("...", scale);

    while (remaining > 0) {
        const char* prev = p;
        size_t prevRem = remaining;
        DecodeUtf8Codepoint(p, remaining);
        size_t charLen = prevRem - remaining;

        clipped.append(prev, charLen);
        if (MeasureTextWidth(clipped, scale) + dotW > maxWidth) {
            clipped += "...";
            break;
        }
    }
    DrawText(clipped, x, y, color, scale, shadow);
}

void Renderer::DrawButtonPrompt(const std::string& buttonName, const std::string& label, float x, float y, uint32_t buttonColor) {
    float badgeR = 10.0f;
    DrawRoundedRect(x, y - 2, badgeR * 2, badgeR * 2, 4.0f, buttonColor);
    DrawText(buttonName, x + 5.0f, y + 1.0f, 0xFFFFFFFF, 1.0f, false);
    DrawText(label, x + badgeR * 2 + 8.0f, y + 2.0f, COLOR_TEXT_PRIMARY, 1.05f, true);
}

void Renderer::DrawBadge(const std::string& text, float x, float y, uint32_t bgColor, uint32_t textColor) {
    float w = MeasureTextWidth(text, 0.85f) + 10.0f;
    float h = 17.0f;
    DrawRoundedRect(x, y, w, h, 3.0f, bgColor);
    DrawText(text, x + 5.0f, y + 1.5f, textColor, 0.85f, false);
}

void Renderer::DrawAvatar(float x, float y, float size, uint32_t color, const std::string& initialLetter) {
    DrawRoundedRect(x, y, size, size, 6.0f, color);
    float textW = MeasureTextWidth(initialLetter, size / 16.0f);
    float textH = GetFontHeight(size / 16.0f);
    DrawText(
        initialLetter,
        x + (size - textW) * 0.5f,
        y + (size - textH) * 0.5f,
        0xFFFFFFFF,
        size / 16.0f,
        true
    );
}

void Renderer::DrawProgressBar(float x, float y, float w, float h, float progress, uint32_t fillColor, uint32_t bgColor) {
    progress = (std::max)(0.0f, (std::min)(1.0f, progress));
    DrawRoundedRect(x, y, w, h, 3.0f, bgColor);
    if (progress > 0.01f) {
        DrawRoundedRect(x, y, w * progress, h, 3.0f, fillColor);
    }
}

} // namespace huEVO
