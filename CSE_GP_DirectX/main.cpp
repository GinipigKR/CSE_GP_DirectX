#include <iostream>

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

//정점 수를 미리 정의해둠.
#define VERTEX_COUNT 6

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pImmediateContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

const char* shaderSource = R"(
struct VS_INPUT { float3 pos : POSITION; float4 col : COLOR; };
struct PS_INPUT { float4 pos : SV_POSITION; float4 col : COLOR; };

PS_INPUT VS(VS_INPUT input) {
    PS_INPUT output;
    output.pos = float4(input.pos, 1.0f); // 3D 좌표를 4D로 확장
    output.col = input.col;
    return output;
}

float4 PS(PS_INPUT input) : SV_Target {
    return input.col; // 정점에서 계산된 색상을 픽셀에 그대로 적용
}
)";


typedef struct {
    //육망성의 중심점
    float posX;
    float posY;

    //중심점을 사용자 입력을 통해 조정하기 위한 변수
    bool vk_up;
    bool vk_down;
    bool vk_right;
    bool vk_left;
    bool wm_lButtonDown;

}GameContext;

GameContext gameContext = { 0.0f,0.0f,false,false,false,false };

//게임 루프 중 Update Game State
//윈도우 프로시저로 설정된 bool 값을 통해 실제 pos 값을 설정
void Update() {
    if (gameContext.vk_up) {
        gameContext.posY += 0.005f;
    }
    if (gameContext.vk_down) {
        gameContext.posY -= 0.005f;
    }
    if (gameContext.vk_right) {
        gameContext.posX += 0.005f;
    }
    if (gameContext.vk_left) {
        gameContext.posX -= 0.005f;
    }
    if (gameContext.wm_lButtonDown) {
        gameContext.posX = 0.0f;
        gameContext.posY = 0.0f;
    }
}


void Render(Vertex vertices[VERTEX_COUNT], ID3D11InputLayout* pInputLayout, ID3D11VertexShader* vShader, ID3D11PixelShader* pShader) {
    //화면 비율로 인해 찌그러짐을 고려한 적절한 육망성의 정점 정의

    //첫 번째 삼각형
    vertices[0] = { 0.0f + gameContext.posX,  0.5f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };
    vertices[1] = { 0.325f + gameContext.posX, -0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };
    vertices[2] = { -0.325f + gameContext.posX, -0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };

    //두 번째 삼각형
    vertices[3] = { 0.0f + gameContext.posX,  -0.5f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };
    vertices[4] = { -0.325f + gameContext.posX, 0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };
    vertices[5] = { 0.325f + gameContext.posX, 0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f };

    //설정한 정점을 기준으로 버퍼 생성
    ID3D11Buffer* pVBuffer;
    D3D11_BUFFER_DESC bd = { sizeof(Vertex) * VERTEX_COUNT, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
    g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);
   
    float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
    g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
    D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
    g_pImmediateContext->RSSetViewports(1, &vp);

    g_pImmediateContext->IASetInputLayout(pInputLayout);
    UINT stride = sizeof(Vertex), offset = 0;
    g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

    g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
    g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

    g_pImmediateContext->Draw(VERTEX_COUNT, 0);

    g_pSwapChain->Present(0, 0);

    //루프 안에서 버퍼를 Create 하였으므로, 루프 마지막에 Release해줌
    if (pVBuffer) pVBuffer->Release();
}


//게임 루프 중 Process Input에 속함.
//전역변수로 선언된 gameContext의 버튼 클릭 여부를 설정
//자주 화면을 벗어나서, 원점으로 되돌리기 위한 마우스 클릭 여부 설정
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)
    {
    case WM_KEYDOWN:

        if (wParam == VK_LEFT || wParam == 'A') {
            gameContext.vk_left = true;
        }
        if (wParam == VK_RIGHT || wParam == 'D') {
            gameContext.vk_right = true;
        }
        if (wParam == VK_UP || wParam == 'W') {
            gameContext.vk_up = true;
        }
        if (wParam == VK_DOWN || wParam == 'S') {
            gameContext.vk_down = true;
        }
        if (wParam == 'Q') {
            PostQuitMessage(0);
        }
        break;

    case WM_KEYUP:
        if (wParam == VK_LEFT || wParam == 'A') {
            gameContext.vk_left = false;
        }
        if (wParam == VK_RIGHT || wParam == 'D') {
            gameContext.vk_right = false;
        }
        if (wParam == VK_UP || wParam == 'W') {
            gameContext.vk_up = false;
        }
        if (wParam == VK_DOWN || wParam == 'S') {
            gameContext.vk_down = false;
        }
        break;

    case WM_LBUTTONDOWN:
        gameContext.wm_lButtonDown = true;
        break;
    case WM_LBUTTONUP:
        gameContext.wm_lButtonDown = false;
        break;
    case WM_RBUTTONDOWN:
        printf("[MOUSE] 오른쪽 클릭됨!\n");
        break;
    case WM_DESTROY:
        printf("[SYSTEM] 윈도우 파괴 메시지 수신. 루프를 탈출합니다.\n");
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    printf("[SYSTEM] 마우스 왼쪽 클릭을 통해 육망성을 원점에 위치시킬 수 있습니다.\n");
    printf("[SYSTEM] 키보드의 WASD와 방향키로 육망성의 위치를 변경시킬 수 있습니다.\n");

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.lpszClassName = L"DX11GameLoopClass";
    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(L"DX11GameLoopClass", L"과제: 움직이는 육망성 만들기",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return -1;
    ShowWindow(hWnd, nCmdShow);

    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();

    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

    ID3D11VertexShader* vShader;
    ID3D11PixelShader* pShader;
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);


    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* pInputLayout;
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
    vsBlob->Release(); psBlob->Release();


    //정점 배열을 미리 선언
    Vertex vertices[VERTEX_COUNT];

    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            //게임루프 중 Process Input에 속하는듯
            //어디서부터 어디까지가 Process Input인가
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            //게임 루프 중 Update
            Update();

            //게임 루프 중 Render
            Render(vertices, pInputLayout, vShader, pShader);

        }
    }

    if (pInputLayout) pInputLayout->Release();
    if (vShader) vShader->Release();
    if (pShader) pShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    return (int)msg.wParam;
}