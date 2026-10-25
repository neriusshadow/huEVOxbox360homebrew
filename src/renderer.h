#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <string>
#include <vector>
#include <cstdint>

#ifdef _XBOX
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
typedef D3DDevice* LPDIRECT3DDEVICE9;
typedef D3DTexture* LPDIRECT3DTEXTURE9;
typedef Direct3D* LPDIRECT3D9;
typedef D3DVertexShader* LPDIRECT3DVERTEXSHADER9;
typedef D3DPixelShader* LPDIRECT3DPIXELSHADER9;
typedef D3DVertexDeclaration* LPDIRECT3DVERTEXDECLARATION9;
#else
typedef void* LPDIRECT3D9;
typedef void* LPDIRECT3DDEVICE9;
typedef void* LPDIRECT3DTEXTURE9;
typedef void* LPDIRECT3DVERTEXSHADER9;
typedef void* LPDIRECT3DPIXELSHADER9;
typedef void* LPDIRECT3DVERTEXDECLARATION9;
typedef uint32_t D3DCOLOR;
#define D3DCOLOR_ARGB(a,r,g,b) \
    ((D3DCOLOR)((((a)&0xff)<<24)|(((r)&0xff)<<16)|(((g)&0xff)<<8)|((b)&0xff)))
#endif

namespace huEVO {

struct Vertex2D {
    float x, y, z, rhw;
    uint32_t color;
    float u, v;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool Initialize();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    // Geometric Drawing Primitives
    void DrawRect(float x, float y, float w, float h, uint32_t color);
    void DrawRoundedRect(float x, float y, float w, float h, float radius, uint32_t fillColor, uint32_t borderColor = 0, float borderWidth = 1.0f);
    void DrawLine(float x1, float y1, float x2, float y2, uint32_t color, float thickness = 1.0f);
    void DrawGradientV(float x, float y, float w, float h, uint32_t topColor, uint32_t bottomColor);
    void DrawDivider(float x, float y, float w, uint32_t color = 0xFF2A2B2E);

    // Text & Font Rendering (Full Russian Cyrillic UTF-8 + Latin + Symbols)
    void DrawText(const std::string& text, float x, float y, uint32_t color, float scale = 1.0f, bool shadow = true);
    void DrawTextClipped(const std::string& text, float x, float y, float maxWidth, uint32_t color, float scale = 1.0f, bool shadow = true);
    float MeasureTextWidth(const std::string& text, float scale = 1.0f);
    float GetFontHeight(float scale = 1.0f);

    // High Level TV/Xbox UI Widgets
    void DrawButtonPrompt(const std::string& buttonName, const std::string& label, float x, float y, uint32_t buttonColor);
    void DrawBadge(const std::string& text, float x, float y, uint32_t bgColor, uint32_t textColor);
    void DrawAvatar(float x, float y, float size, uint32_t color, const std::string& initialLetter);
    void DrawProgressBar(float x, float y, float w, float h, float progress, uint32_t fillColor, uint32_t bgColor);

    LPDIRECT3DDEVICE9 GetDevice() const { return m_pDevice; }

private:
    void FlushBatch();

    LPDIRECT3D9 m_pD3D;
    LPDIRECT3DDEVICE9 m_pDevice;
    LPDIRECT3DVERTEXSHADER9 m_pVertexShader;
    LPDIRECT3DPIXELSHADER9 m_pPixelShader;
    LPDIRECT3DVERTEXDECLARATION9 m_pVertexDecl;

    std::vector<Vertex2D> m_VertexBuffer;
    bool m_InFrame;
};

extern Renderer g_Renderer;

} // namespace huEVO
