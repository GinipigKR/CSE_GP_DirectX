/*
 * [강의 노트: DirectX 11 & Win32 GameLoop]
 * 1. WinMain: 프로그램의 입구
 * 2. WndProc: OS가 보낸 우편물(메시지)을 확인하는 곳
 * 3. GameLoop: 쉬지 않고 Update와 Render를 반복하는 엔진의 심장
 * 4. Release: 빌려온 GPU 메모리를 반드시 반납하는 습관 (메모리 누수 방지)
 */
#include <iostream>

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

 // --- [전역 객체 관리] ---
 // DirectX 객체들은 GPU 메모리를 직접 사용함. 
 // 사용 후 'Release()'를 호출하지 않으면 프로그램 종료 후에도 메모리가 점유될 수 있음(메모리 누수).
ID3D11Device* g_pd3dDevice = nullptr;          // 리소스 생성자 (공장)
ID3D11DeviceContext* g_pImmediateContext = nullptr;   // 그리기 명령 수행 (일꾼)
IDXGISwapChain* g_pSwapChain = nullptr;          // 화면 전환 (더블 버퍼링)
ID3D11RenderTargetView* g_pRenderTargetView = nullptr;   // 그림을 그릴 도화지(View)

struct Vertex {
    float x, y, z;
    float r, g, b, a;
};

// HLSL (High-Level Shading Language) 소스
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
    float posX;
    float posY;

    bool vk_up;
    bool vk_down;
    bool vk_right;
    bool vk_left;
    bool wm_lButtonDown;

}GameContext;

GameContext gameContext = { 0.0f,0.0f,false,false,false,false }; 

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

void Render() {

}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message)    //윈도우의 이벤트는 메시지에 담김
    {
        // --- [키보드 메시지 처리] ---
        // WM_KEYDOWN: 키보드가 눌리는 '사건'이 발생했을 때 OS가 호출함.
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
            PostQuitMessage(0); // 메시지 큐에 WM_QUIT을 넣음
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

        // --- [시스템 메시지 처리] ---
    case WM_DESTROY:
        // 사용자가 'X' 버튼을 눌러 창을 닫으려 할 때 호출됨.
        printf("[SYSTEM] 윈도우 파괴 메시지 수신. 루프를 탈출합니다.\n");
        PostQuitMessage(0);
        break;

    default:
        // 우리가 관심 없는 메시지(창 크기 조절, 포커스 변경 등)는 OS가 기본값으로 처리함.
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 1. 윈도우 등록 및 생성
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

    // 2. DX11 디바이스 및 스왑 체인 초기화
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800; sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;

    // GPU와 통신할 통로(Device)와 화면(SwapChain)을 생성함.
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
        D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);

    // 렌더 타겟 설정 (도화지 준비)
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release(); // 뷰를 생성했으므로 원본 텍스트는 바로 해제 (중요!)

    // 3. 셰이더 컴파일 및 생성
    ID3DBlob* vsBlob, * psBlob;
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "VS", "vs_4_0", 0, 0, &vsBlob, nullptr);
    D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, "PS", "ps_4_0", 0, 0, &psBlob, nullptr);

    ID3D11VertexShader* vShader;
    ID3D11PixelShader* pShader;
    g_pd3dDevice->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &vShader);
    g_pd3dDevice->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pShader);


    // 정점의 데이터 형식을 정의 (IA 단계에 알려줌)
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* pInputLayout;
    g_pd3dDevice->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pInputLayout);
    vsBlob->Release(); psBlob->Release(); // 컴파일용 임시 메모리 해제

    

    // --- [5. 정석 게임 루프] ---
    MSG msg = { 0 };
    while (WM_QUIT != msg.message) {
        // (1) 입력 단계: PeekMessage는 메시지가 없어도 바로 리턴함 (Non-blocking)
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            // (2) 업데이트 단계: 여기서 캐릭터의 위치나 로직을 계산함
            // (과제: GetAsyncKeyState 등을 써서 posX, posY를 변경하셈)
            Update();

            //여기부터 내가 ========================================

            // 4. 정점 버퍼 생성 (삼각형 데이터)
            Vertex vertices[] = {
                {  0.0f + gameContext.posX,  0.5f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
                {  0.325f + gameContext.posX, -0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
                { -0.325f + gameContext.posX, -0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },

                {  0.0f + gameContext.posX,  -0.5f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
                { -0.325f + gameContext.posX, 0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
                {  0.325f + gameContext.posX, 0.25f + gameContext.posY, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f },
                //순서가 바뀌면 렌더링이 안된다고 함.
            };
            ID3D11Buffer* pVBuffer;
            D3D11_BUFFER_DESC bd = { sizeof(vertices), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
            D3D11_SUBRESOURCE_DATA initData = { vertices, 0, 0 };
            g_pd3dDevice->CreateBuffer(&bd, &initData, &pVBuffer);


            //여기까지 내가 =========================================

            // (3) 출력 단계: 변한 데이터를 바탕으로 화면에 그림
            float clearColor[] = { 0.1f, 0.2f, 0.3f, 1.0f };
            g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);

            // 렌더링 파이프라인 상태 설정
            g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, nullptr);
            D3D11_VIEWPORT vp = { 0, 0, 800, 600, 0.0f, 1.0f };
            g_pImmediateContext->RSSetViewports(1, &vp);

            g_pImmediateContext->IASetInputLayout(pInputLayout);
            UINT stride = sizeof(Vertex), offset = 0;
            g_pImmediateContext->IASetVertexBuffers(0, 1, &pVBuffer, &stride, &offset);

            // Primitive Topology 설정: 삼각형 리스트로 연결하라!
            g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            g_pImmediateContext->VSSetShader(vShader, nullptr, 0);
            g_pImmediateContext->PSSetShader(pShader, nullptr, 0);

            // 최종 그리기
            g_pImmediateContext->Draw(6, 0);

            // 화면 교체 (프론트 버퍼와 백 버퍼 스왑)
            g_pSwapChain->Present(0, 0);

            if (pVBuffer) pVBuffer->Release();
        }
    }

    // --- [6. 자원 해제 (Release)] ---
    // 생성(Create)한 모든 객체는 프로그램 종료 전 반드시 Release 해야 함.
    // 생성의 역순으로 해제하는 것이 관례임.
    
    if (pInputLayout) pInputLayout->Release();
    if (vShader) vShader->Release();
    if (pShader) pShader->Release();
    if (g_pRenderTargetView) g_pRenderTargetView->Release();
    if (g_pSwapChain) g_pSwapChain->Release();
    if (g_pImmediateContext) g_pImmediateContext->Release();
    if (g_pd3dDevice) g_pd3dDevice->Release();

    return (int)msg.wParam;
}