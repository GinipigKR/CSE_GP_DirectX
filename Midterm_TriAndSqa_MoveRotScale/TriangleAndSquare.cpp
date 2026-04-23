#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <iostream>
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <chrono>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// ============================================================
// [전역 DirectX 리소스]
// ============================================================
ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;
ID3D11VertexShader* g_pVertexShader = nullptr;
ID3D11PixelShader* g_pPixelShader = nullptr;
ID3D11InputLayout* g_pInputLayout = nullptr;
ID3D11Buffer* g_pConstantBuffer = nullptr;

// ============================================================
// [상수 버퍼 데이터]
// ============================================================
struct ConstantData
{
    DirectX::XMFLOAT2 position; // 8바이트
    float angle;                // 4바이트
    float pad0;                 // 4바이트 -> 첫 16바이트 정렬 완성

    DirectX::XMFLOAT2 scale;    // 8바이트
    float pad1[2];              // 8바이트 -> 총 32바이트
};

struct Vertex
{
    float x, y, z;
    float r, g, b, a;
};

struct VideoConfig
{
    int Width = 800;
    int Height = 600;
    bool IsFullscreen = false;
    bool NeedsResize = false;
    int VSync = 1;
} g_Config;

struct InputContext
{
    // WASD 이동
    bool keyW = false;
    bool keyA = false;
    bool keyS = false;
    bool keyD = false;

    // 방향키 이동
    bool keyUp = false;
    bool keyLeft = false;
    bool keyDown = false;
    bool keyRight = false;

    // 방향키 오브젝트 회전
    bool key1 = false;
    bool key2 = false;

    // 방향키 오브젝트 스케일
    bool key3 = false;
    bool key4 = false;

    // WASD 오브젝트 회전
    bool key5 = false;
    bool key6 = false;

    // WASD 오브젝트 스케일
    bool key7 = false;
    bool key8 = false;

} inputContext;

// ============================================================
// [렌더 타겟 재생성]
// ============================================================
void RebuildVideoResources(HWND hWnd)
{
    if (!g_pSwapChain) return;

    if (g_pRenderTargetView)
    {
        g_pRenderTargetView->Release();
        g_pRenderTargetView = nullptr;
    }

    g_pSwapChain->ResizeBuffers(0, g_Config.Width, g_Config.Height, DXGI_FORMAT_UNKNOWN, 0);

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    if (pBackBuffer == 0) {
        printf("asd");
        return;
    }
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();

    if (!g_Config.IsFullscreen)
    {
        RECT rc = { 0, 0, g_Config.Width, g_Config.Height };
        AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
        SetWindowPos(
            hWnd,
            nullptr,
            0,
            0,
            rc.right - rc.left,
            rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOZORDER
        );
    }

    g_Config.NeedsResize = false;
    printf("[Video] Changed: %d x %d\n", g_Config.Width, g_Config.Height);
}

void ToggleFullscreen()
{
    if (!g_pSwapChain) return;

    g_Config.IsFullscreen = !g_Config.IsFullscreen;
    g_pSwapChain->SetFullscreenState(g_Config.IsFullscreen, nullptr);
}

// ============================================================
// [게임 오브젝트 시스템]
// ============================================================
class GameObject;

class Component
{
public:
    GameObject* pOwner = nullptr;
    bool isStarted = false;

    virtual void Start() = 0;
    virtual void Input() {}
    virtual void Update(float dt) {}
    virtual void Render() {}
    virtual ~Component() {}
};

struct Transform
{
    float posX = 0.0f;
    float posY = 0.0f;
    float rotation = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
};

class GameObject
{
public:
    std::string name;
    Transform transform;
    std::vector<Component*> components;

    GameObject(const std::string& objectName)
        : name(objectName)
    {
    }

    void AddComponent(Component* pComp)
    {
        pComp->pOwner = this;
        pComp->isStarted = false;
        components.push_back(pComp);
    }

    ~GameObject()
    {
        for (Component* c : components)
        {
            delete c;
        }
        components.clear();
    }
};

// ============================================================
// [이동 컴포넌트]
// - isArrow == true  : 방향키 + 1,2,3,4
// - isArrow == false : WASD + 5,6,7,8
// ============================================================
class MoveComponent : public Component
{
private:
    bool isArrow = false;
    float moveSpeed = 1.0f;

    float velX = 0.0f;
    float velY = 0.0f;
    float angleVel = 0.0f;
    float scaleVelX = 0.0f;
    float scaleVelY = 0.0f;

public:
    MoveComponent(bool useArrowInput, float speed)
        : isArrow(useArrowInput), moveSpeed(speed)
    {
    }

    void Start() override
    {
    }

    void Input() override
    {
        velX = 0.0f;
        velY = 0.0f;
        angleVel = 0.0f;
        scaleVelX = 0.0f;
        scaleVelY = 0.0f;

        if (isArrow)
        {
            // 방향키 이동
            if (inputContext.keyLeft)  velX = -moveSpeed;
            if (inputContext.keyRight) velX = +moveSpeed;
            if (inputContext.keyUp)    velY = +moveSpeed;
            if (inputContext.keyDown)  velY = -moveSpeed;

            // 회전
            if (inputContext.key1) angleVel += 2.0f;
            if (inputContext.key2) angleVel -= 2.0f;

            // 스케일
            if (inputContext.key3)
            {
                scaleVelX += 1.0f;
                scaleVelY += 1.0f;
            }
            if (inputContext.key4)
            {
                scaleVelX -= 1.0f;
                scaleVelY -= 1.0f;
            }
        }
        else
        {
            // WASD 이동
            if (inputContext.keyA) velX = -moveSpeed;
            if (inputContext.keyD) velX = +moveSpeed;
            if (inputContext.keyW) velY = +moveSpeed;
            if (inputContext.keyS) velY = -moveSpeed;

            // 회전
            if (inputContext.key5) angleVel += 2.0f;
            if (inputContext.key6) angleVel -= 2.0f;

            // 스케일
            if (inputContext.key7)
            {
                scaleVelX += 1.0f;
                scaleVelY += 1.0f;
            }
            if (inputContext.key8)
            {
                scaleVelX -= 1.0f;
                scaleVelY -= 1.0f;
            }
        }
    }

    void Update(float dt) override
    {
        pOwner->transform.posX += velX * dt;
        pOwner->transform.posY += velY * dt;
        pOwner->transform.rotation += angleVel * dt;
        pOwner->transform.scaleX += scaleVelX * dt;
        pOwner->transform.scaleY += scaleVelY * dt;

        // 화면 범위 제한
        if (pOwner->transform.posX < -0.95f) pOwner->transform.posX = -0.95f;
        if (pOwner->transform.posX >  0.95f) pOwner->transform.posX =  0.95f;
        if (pOwner->transform.posY < -0.95f) pOwner->transform.posY = -0.95f;
        if (pOwner->transform.posY >  0.95f) pOwner->transform.posY =  0.95f;

        // 스케일 최소값 제한
        if (pOwner->transform.scaleX < 0.1f) pOwner->transform.scaleX = 0.1f;
        if (pOwner->transform.scaleY < 0.1f) pOwner->transform.scaleY = 0.1f;
    }
};

// ============================================================
// [렌더링 컴포넌트]
// - owner Transform을 ConstantData로 패키징
// - register(b0)로 전달 후 Draw
// ============================================================
class RenderComponent : public Component
{
private:
    ID3D11Buffer* m_vertexBuffer = nullptr;
    UINT m_vertexCount = 0;

public:
    RenderComponent(const Vertex* vertices, UINT vertexCount)
        : m_vertexCount(vertexCount)
    {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(Vertex) * vertexCount;
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bd.CPUAccessFlags = 0;
        bd.MiscFlags = 0;
        bd.StructureByteStride = 0;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = vertices;

        g_pd3dDevice->CreateBuffer(&bd, &initData, &m_vertexBuffer);
    }

    void Start() override
    {
    }

    void Render() override
    {
        ConstantData cbData = {};
        cbData.position = DirectX::XMFLOAT2(pOwner->transform.posX, pOwner->transform.posY);
        cbData.angle = pOwner->transform.rotation;
        cbData.scale = DirectX::XMFLOAT2(pOwner->transform.scaleX, pOwner->transform.scaleY);

        g_pImmediateContext->UpdateSubresource(g_pConstantBuffer, 0, nullptr, &cbData, 0, 0);
        g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        g_pImmediateContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);
        g_pImmediateContext->IASetInputLayout(g_pInputLayout);
        g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        g_pImmediateContext->VSSetShader(g_pVertexShader, nullptr, 0);
        g_pImmediateContext->PSSetShader(g_pPixelShader, nullptr, 0);
        g_pImmediateContext->Draw(m_vertexCount, 0);
    }

    ~RenderComponent()
    {
        if (m_vertexBuffer)
        {
            m_vertexBuffer->Release();
            m_vertexBuffer = nullptr;
        }
    }
};

// ============================================================
// [GameLoop]
// - WinMain의 DirectX 루프를 이 안으로 옮긴 버전
// ============================================================
class GameLoop
{
public:
    bool isRunning = true;
    std::vector<GameObject*> gameWorld;
    std::chrono::high_resolution_clock::time_point prevTime;
    float deltaTime = 0.0f;

    void Initialize()
    {
        isRunning = true;
        prevTime = std::chrono::high_resolution_clock::now();
        deltaTime = 0.0f;
    }

    void TickTime()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = currentTime - prevTime;
        deltaTime = elapsed.count();
        prevTime = currentTime;
    }

    void Input()
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
            isRunning = false;

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
        // Start 1회 보장
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

        // 실제 Update
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
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            for (int j = 0; j < (int)gameWorld[i]->components.size(); j++)
            {
                gameWorld[i]->components[j]->Render();
            }
        }
    }

    void Run(HWND hWnd)
    {
        MSG msg = { 0 };

        while (WM_QUIT != msg.message && isRunning)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else
            {
                // -------------------------
                // delta time 계산
                // -------------------------
                TickTime();

                // -------------------------
                // 1회성 입력 처리
                // -------------------------
                if (GetAsyncKeyState('F') & 0x0001)
                {
                    ToggleFullscreen();
                }
                if (GetAsyncKeyState('9') & 0x0001)
                {
                    g_Config.Width = 800;
                    g_Config.Height = 600;
                    g_Config.NeedsResize = true;
                }
                if (GetAsyncKeyState('0') & 0x0001)
                {
                    g_Config.Width = 1280;
                    g_Config.Height = 720;
                    g_Config.NeedsResize = true;
                }

                if (g_Config.NeedsResize)
                {
                    RebuildVideoResources(hWnd);
                }

                // -------------------------
                // Input / Update
                // -------------------------
                Input();
                Update();

                // -------------------------
                // Render Begin
                // -------------------------
                float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
                g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

                D3D11_VIEWPORT vp = {};
                vp.TopLeftX = 0.0f;
                vp.TopLeftY = 0.0f;
                vp.Width = static_cast<float>(g_Config.Width);
                vp.Height = static_cast<float>(g_Config.Height);
                vp.MinDepth = 0.0f;
                vp.MaxDepth = 1.0f;

                g_pImmediateContext->RSSetViewports(1, &vp);
                g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);

                // -------------------------
                // GameWorld Render
                // -------------------------
                Render();

                // -------------------------
                // Present
                // -------------------------
                g_pSwapChain->Present(g_Config.VSync, 0);
            }
        }
    }

    ~GameLoop()
    {
        for (int i = 0; i < (int)gameWorld.size(); i++)
        {
            delete gameWorld[i];
        }
        gameWorld.clear();
    }
};

// ============================================================
// [윈도우 메시지 처리]
// ============================================================
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        switch (wParam)
        {
        case 'W': inputContext.keyW = true; break;
        case 'A': inputContext.keyA = true; break;
        case 'S': inputContext.keyS = true; break;
        case 'D': inputContext.keyD = true; break;

        case VK_UP:    inputContext.keyUp = true; break;
        case VK_LEFT:  inputContext.keyLeft = true; break;
        case VK_DOWN:  inputContext.keyDown = true; break;
        case VK_RIGHT: inputContext.keyRight = true; break;

        case '1': inputContext.key1 = true; break;
        case '2': inputContext.key2 = true; break;
        case '3': inputContext.key3 = true; break;
        case '4': inputContext.key4 = true; break;

        case '5': inputContext.key5 = true; break;
        case '6': inputContext.key6 = true; break;
        case '7': inputContext.key7 = true; break;
        case '8': inputContext.key8 = true; break;
        }
        return 0;

    case WM_KEYUP:
        switch (wParam)
        {
        case 'W': inputContext.keyW = false; break;
        case 'A': inputContext.keyA = false; break;
        case 'S': inputContext.keyS = false; break;
        case 'D': inputContext.keyD = false; break;

        case VK_UP:    inputContext.keyUp = false; break;
        case VK_LEFT:  inputContext.keyLeft = false; break;
        case VK_DOWN:  inputContext.keyDown = false; break;
        case VK_RIGHT: inputContext.keyRight = false; break;

        case '1': inputContext.key1 = false; break;
        case '2': inputContext.key2 = false; break;
        case '3': inputContext.key3 = false; break;
        case '4': inputContext.key4 = false; break;

        case '5': inputContext.key5 = false; break;
        case '6': inputContext.key6 = false; break;
        case '7': inputContext.key7 = false; break;
        case '8': inputContext.key8 = false; break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

// ============================================================
// [엔트리 포인트]
// ============================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // ========================================================
    // [1] 윈도우 클래스 등록 및 창 생성
    // ========================================================
    WNDCLASSEXW wcex = { sizeof(WNDCLASSEXW) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.lpszClassName = L"DX11GameObjectClass";
    RegisterClassExW(&wcex);

    RECT rc = { 0, 0, g_Config.Width, g_Config.Height };
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hWnd = CreateWindowW(
        L"DX11GameObjectClass",
        L"DX11 Component GameObject Sample",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr, nullptr, hInstance, nullptr
    );

    ShowWindow(hWnd, nCmdShow);

    // ========================================================
    // [2] Device / SwapChain 생성
    // ========================================================
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = g_Config.Width;
    sd.BufferDesc.Height = g_Config.Height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        nullptr,
        &g_pImmediateContext
    );

    // ========================================================
    // [3] RenderTargetView 생성
    // ========================================================
    RebuildVideoResources(hWnd);

    // ========================================================
    // [4] 셰이더 소스
    // ========================================================
    const char* shaderSource = R"(
cbuffer TransformBuffer : register(b0)
{
    float2 g_Position;
    float  g_Angle;
    float  g_Pad0;

    float2 g_Scale;
    float2 g_Pad1;
};

struct VS_INPUT
{
    float3 pos : POSITION;
    float4 col : COLOR;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

PS_INPUT VS_Main(VS_INPUT input)
{
    PS_INPUT output;

    float2 p = input.pos.xy;

    // Scale
    p.x *= g_Scale.x;
    p.y *= g_Scale.y;

    // Rotate
    float c = cos(g_Angle);
    float s = sin(g_Angle);

    float2 rotated;
    rotated.x = p.x * c - p.y * s;
    rotated.y = p.x * s + p.y * c;

    // Translate
    rotated += g_Position;

    output.pos = float4(rotated, input.pos.z, 1.0f);
    output.col = input.col;
    return output;
}

float4 PS_Main(PS_INPUT input) : SV_Target
{
    return input.col;
}
)";

    ID3DBlob* vsBlob = nullptr;
    ID3DBlob* psBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "VS_Main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);

    if (errorBlob)
    {
        std::cout << (char*)errorBlob->GetBufferPointer() << std::endl;
        errorBlob->Release();
        errorBlob = nullptr;
    }

    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr,
        "PS_Main", "ps_4_0", 0, 0, &psBlob, &errorBlob);

    if (errorBlob)
    {
        std::cout << (char*)errorBlob->GetBufferPointer() << std::endl;
        errorBlob->Release();
        errorBlob = nullptr;
    }

    g_pd3dDevice->CreateVertexShader(
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        nullptr,
        &g_pVertexShader
    );

    g_pd3dDevice->CreatePixelShader(
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize(),
        nullptr,
        &g_pPixelShader
    );

    // ========================================================
    // [5] 입력 레이아웃 생성
    // ========================================================
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    g_pd3dDevice->CreateInputLayout(
        layout,
        2,
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize(),
        &g_pInputLayout
    );

    vsBlob->Release();
    psBlob->Release();

    // ========================================================
    // [6] Constant Buffer 생성
    // ========================================================
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(ConstantData);
    cbDesc.Usage = D3D11_USAGE_DEFAULT;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = 0;
    cbDesc.MiscFlags = 0;
    cbDesc.StructureByteStride = 0;

    g_pd3dDevice->CreateBuffer(&cbDesc, nullptr, &g_pConstantBuffer);

    // ========================================================
    // [7] 도형 정점 데이터 준비
    // ========================================================
    Vertex triangleVertices[] =
    {
        {  0.0f,  0.2f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
        {  0.2f, -0.2f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f },
        { -0.2f, -0.2f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f },
    };

    Vertex rectVertices[] =
    {
        { -0.2f,  0.2f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f },
        {  0.2f,  0.2f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f },
        {  0.2f, -0.2f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },

        { -0.2f,  0.2f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f },
        {  0.2f, -0.2f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f },
        { -0.2f, -0.2f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
    };

    // ========================================================
    // [8] GameObject 생성
    // ========================================================
    GameObject* triangleObj = new GameObject("Triangle");
    triangleObj->transform.posX = -0.5f;
    triangleObj->transform.posY = 0.0f;
    triangleObj->AddComponent(new MoveComponent(false, 1.0f));   // WASD
    triangleObj->AddComponent(new RenderComponent(triangleVertices, 3));

    GameObject* rectObj = new GameObject("Rectangle");
    rectObj->transform.posX = 0.5f;
    rectObj->transform.posY = 0.0f;
    rectObj->AddComponent(new MoveComponent(true, 1.0f));        // 방향키
    rectObj->AddComponent(new RenderComponent(rectVertices, 6));

    // ========================================================
    // [9] GameLoop 생성 및 월드 등록
    // ========================================================
    GameLoop* gameworld = new GameLoop();
    gameworld->Initialize();
    gameworld->gameWorld.push_back(triangleObj);
    gameworld->gameWorld.push_back(rectObj);

    // ========================================================
    // [10] 엔진 메인 루프 실행
    // ========================================================
    gameworld->Run(hWnd);

    // ========================================================
    // [11] 정리
    // ========================================================
    delete gameworld;

    if (g_pConstantBuffer) { g_pConstantBuffer->Release(); g_pConstantBuffer = nullptr; }
    if (g_pInputLayout) { g_pInputLayout->Release(); g_pInputLayout = nullptr; }
    if (g_pVertexShader) { g_pVertexShader->Release(); g_pVertexShader = nullptr; }
    if (g_pPixelShader) { g_pPixelShader->Release(); g_pPixelShader = nullptr; }
    if (g_pRenderTargetView) { g_pRenderTargetView->Release(); g_pRenderTargetView = nullptr; }
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pImmediateContext) { g_pImmediateContext->Release(); g_pImmediateContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }

    return 0;
}