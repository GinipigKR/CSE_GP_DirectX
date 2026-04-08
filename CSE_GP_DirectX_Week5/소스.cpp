#include <iostream>
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <cstring>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define TRIANGLE_VERTEX_COUNT 3

struct VideoConfig
{
    int Width = 800;
    int Height = 600;
    bool IsFullscreen = false;
    bool NeedsResize = false;
    int VSync = 1;
} g_Config;

// 전역 변수
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;     
ID3D11PixelShader* g_pPixelShader = nullptr;       
ID3D11InputLayout* g_pInputLayout = nullptr;       
HWND g_hWnd = nullptr;                             

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f);
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col;
}
)";

struct InputContext {
    bool keyW = false;
    bool keyA = false;
    bool keyS = false;
    bool keyD = false;

    bool keyUp = false;
    bool keyLeft = false;
    bool keyDown = false;
    bool keyRight = false;
};
InputContext inputCtx;

void ToggleFullscreen()
{
    if (!g_pSwapChain) return;

    g_Config.IsFullscreen = !g_Config.IsFullscreen;
    g_pSwapChain->SetFullscreenState(g_Config.IsFullscreen, nullptr);
}

void DrawTriangle(const Vertex vertices[TRIANGLE_VERTEX_COUNT])
{
    ID3D11Buffer* pVBuffer = nullptr;

    D3D11_BUFFER_DESC bd = { sizeof(Vertex) * TRIANGLE_VERTEX_COUNT, D3D11_USAGE_DEFAULT ,D3D11_BIND_VERTEX_BUFFER ,0,0,0};
    D3D11_SUBRESOURCE_DATA initData = { vertices ,0,0};
    g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);
    g_pImmediateContext->Draw(TRIANGLE_VERTEX_COUNT, 0);

    pVBuffer->Release();
}

class GameObject;

class Component
{
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() {}
    virtual void Input() {}
    virtual void Update(float dt) {}
    virtual void Render() {}
    virtual ~Component() {}
};

class GameObject {
public:
    std::string name;
    float posX = 0.0f;
    float posY = 0.0f;
    float velX = 0.0f;
    float velY = 0.0f;
    std::vector<Component*> components;

    GameObject(std::string n) : name(n) {}

    ~GameObject() {
        for (int i = 0; i < (int)components.size(); i++)
        {
            delete components[i];
        }
    }

    void AddComponent(Component* pComp)
    {
        pComp->pOwner = this;
        pComp->isStarted = false;
        components.push_back(pComp);
    }
};

enum class ControlType
{
    ArrowKeys,
    WASD
};

class MoveComponent : public Component
{
private:
    ControlType controlType;
    float moveSpeed;

public:
    MoveComponent(ControlType type, float speed)
        : controlType(type), moveSpeed(speed) {
    }

    void Input() override
    {
        pOwner->velX = 0.0f;
        pOwner->velY = 0.0f;

        if (controlType == ControlType::ArrowKeys)
        {
            if (inputCtx.keyLeft)  pOwner->velX = -moveSpeed;
            if (inputCtx.keyRight) pOwner->velX = +moveSpeed;
            if (inputCtx.keyUp)    pOwner->velY = +moveSpeed;
            if (inputCtx.keyDown)  pOwner->velY = -moveSpeed;
        }
        else if (controlType == ControlType::WASD)
        {
            if (inputCtx.keyA) pOwner->velX = -moveSpeed;
            if (inputCtx.keyD) pOwner->velX = +moveSpeed;
            if (inputCtx.keyW) pOwner->velY = +moveSpeed;
            if (inputCtx.keyS) pOwner->velY = -moveSpeed;
        }
    }

    void Update(float dt) override
    {
        pOwner->posX += pOwner->velX * dt;
        pOwner->posY += pOwner->velY * dt;

        if (pOwner->posX < -0.95f) pOwner->posX = -0.95f;
        if (pOwner->posX > 0.95f)  pOwner->posX = 0.95f;
        if (pOwner->posY < -0.95f) pOwner->posY = -0.95f;
        if (pOwner->posY > 0.95f)  pOwner->posY = 0.95f;
    }
};

class RendererComponent : public Component
{
public:
    virtual void Render() override = 0;
};

class TriangleRenderer : public RendererComponent
{
private:
    Vertex localVertices[TRIANGLE_VERTEX_COUNT];

public:
    TriangleRenderer(const Vertex(&verts)[TRIANGLE_VERTEX_COUNT])
    {
        for (int i = 0; i < TRIANGLE_VERTEX_COUNT; ++i)
        {
            localVertices[i] = verts[i];
        }
    }

    void Render() override
    {
        Vertex worldVertices[TRIANGLE_VERTEX_COUNT];

        for (int i = 0; i < TRIANGLE_VERTEX_COUNT; ++i)
        {
            worldVertices[i] = localVertices[i];
            worldVertices[i].x += pOwner->posX;
            worldVertices[i].y += pOwner->posY;
        }

        DrawTriangle(worldVertices);
    }
};

class GameLoop
{
public:
    bool isRunning;
    std::vector<GameObject*> gameWorld;
    std::chrono::high_resolution_clock::time_point prevTime;
    float deltaTime;

    void Initialize()
    {
        isRunning = true;
        gameWorld.clear();
        prevTime = std::chrono::high_resolution_clock::now();
        deltaTime = 0.0f;
    }

    void Input()
    {
        MSG msg = {};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) isRunning = false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Input();
            }
        }
    }

    void Update()
    {
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                if (gameWorld[i]->components[j]->isStarted == false)
                {
                    gameWorld[i]->components[j]->Start();
                    gameWorld[i]->components[j]->isStarted = true;
                }
            }
        }

        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Update(deltaTime);
            }
        }
    }

    void Render()
    {
        float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
        g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

        D3D11_VIEWPORT vp = { 0.0f, 0.0f, (float)g_Config.Width, (float)g_Config.Height, 0.0f, 1.0f };
        g_pImmediateContext->RSSetViewports(1, &vp);
        g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

        g_pImmediateContext->IASetInputLayout(g_pInputLayout);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);

        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Render();
            }
        }

        g_pSwapChain->Present(g_Config.VSync, 0);
    }

    void Run()
    {
        while (isRunning) {

            std::chrono::high_resolution_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
            std::chrono::duration<float> elapsed = currentTime - prevTime;
            deltaTime = elapsed.count();
            prevTime = currentTime;

            Input();
            Update();
            Render();

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    GameLoop()
    {
        Initialize();
    }

    ~GameLoop()
    {
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            delete gameWorld[i];
        }
    }
};

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_KEYDOWN:
        if (wParam == VK_UP)    inputCtx.keyUp = true;
        if (wParam == VK_LEFT)  inputCtx.keyLeft = true;
        if (wParam == VK_DOWN)  inputCtx.keyDown = true;
        if (wParam == VK_RIGHT) inputCtx.keyRight = true;

        if (wParam == 'W') inputCtx.keyW = true;
        if (wParam == 'A') inputCtx.keyA = true;
        if (wParam == 'S') inputCtx.keyS = true;
        if (wParam == 'D') inputCtx.keyD = true;

        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }

        if (wParam == 'F') {
            ToggleFullscreen();
        }
        break;

    case WM_KEYUP:
        if (wParam == VK_UP)    inputCtx.keyUp = false;
        if (wParam == VK_LEFT)  inputCtx.keyLeft = false;
        if (wParam == VK_DOWN)  inputCtx.keyDown = false;
        if (wParam == VK_RIGHT) inputCtx.keyRight = false;

        if (wParam == 'W') inputCtx.keyW = false;
        if (wParam == 'A') inputCtx.keyA = false;
        if (wParam == 'S') inputCtx.keyS = false;
        if (wParam == 'D') inputCtx.keyD = false;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("[SYSTEM] 방향키: Triangle 1 이동\n");
    printf("[SYSTEM] WASD: Triangle 2 이동\n");
    printf("[SYSTEM] ESC: 종료\n");
    printf("[SYSTEM] F: 전체화면 토글\n");


    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11GameLoopClass";
    RegisterClassExW(&wcex);

    RECT rc = { 0, 0, g_Config.Width, g_Config.Height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindowW(L"DX11GameLoopClass", L"과제 : 각각 움직이는 삼각형",WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr);

    ShowWindow(g_hWnd, nCmdShow);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = g_Config.Width;
    sd.BufferDesc.Height = g_Config.Height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);


    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);

    if (pBackBuffer != nullptr)
    {
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
        pBackBuffer->Release();
    }

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;

    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &g_pVertexShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &g_pPixelShader);

    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &g_pInputLayout);
    vsBlob->Release(); psBlob->Release();

    //게임루프
    GameLoop gLoop;

    Vertex tri1[TRIANGLE_VERTEX_COUNT] = {
        {  0.0f,   0.12f, 0.5f, 1.0f, 0.1f, 0.1f, 1.0f },
        {  0.10f, -0.10f, 0.5f, 1.0f, 0.1f, 0.1f, 1.0f },
        { -0.10f, -0.10f, 0.5f, 1.0f, 0.1f, 0.1f, 1.0f },
    };

    Vertex tri2[TRIANGLE_VERTEX_COUNT] = {
        {  0.0f,   0.12f, 0.5f, 0.1f, 0.4f, 1.0f, 1.0f },
        {  0.10f, -0.10f, 0.5f, 0.1f, 0.4f, 1.0f, 1.0f },
        { -0.10f, -0.10f, 0.5f, 0.1f, 0.4f, 1.0f, 1.0f },
    };

    //삼각형 오브젝트 생성
    GameObject* player1 = new GameObject("Triangle_Object1");
    player1->posX = -0.4f;
    player1->posY = 0.0f;
    player1->AddComponent(new MoveComponent(ControlType::ArrowKeys, 0.8f));
    player1->AddComponent(new TriangleRenderer(tri1));
    gLoop.gameWorld.push_back(player1);

    GameObject* player2 = new GameObject("Triangle_Object2");
    player2->posX = 0.4f;
    player2->posY = 0.0f;
    player2->AddComponent(new MoveComponent(ControlType::WASD, 0.8f));
    player2->AddComponent(new TriangleRenderer(tri2));
    gLoop.gameWorld.push_back(player2);

    gLoop.Run();

    //[중요] 메모리 해제
    if (g_pInputLayout) g_pInputLayout->Release();
    if (g_pVertexShader) g_pVertexShader->Release();
    if (g_pPixelShader) g_pPixelShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    return 0;
}