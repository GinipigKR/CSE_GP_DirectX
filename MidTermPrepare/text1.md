1.Component 클래스의 Start() 함수를 순수 가상 함수로 만들어 자식 클래스에서 반드시 구현하도록 강제하고자 함. 빈칸 (A)에 들어갈 올바른 코드는?
virtual void Start() [(A)]

A. virtual;
B. = 0;
C. NULL;
D. {}

2.GravityBody의 Update 함수에서 중력에 의해 물체가 아래로 떨어지는 로직을 완성하려 함. 빈칸 (B)에 들어갈 수식으로 옳은 것은? (단, 콘솔 좌표계는 아래로 갈수록 y값이 커짐)

void Update(float dt) override {
velocityY += 9.8f * dt;
[(B)]
}

A. y = velocityY;
B. y += 9.8f * dt;
C. y -= velocityY * dt;
D. y += velocityY * dt;
	

3.엔진의 메인 루프가 실행되지 않고 즉시 종료되는 버그를 심으려 함. 빈칸 (C)에 들어갈 잘못된 조건문은?

while ([(C)]) { ... }

A. isRunning == true
B. !isRunning
C. isRunning
D. true


4.GameLoop 클래스의 Run() 함수 내부에 구현된 '1. Start 호출 체크' 로직의 목적은 무엇인가?

A. 매 프레임마다 모든 데이터를 초기 상태로 되돌리기 위함.
B. 객체가 생성된 후 첫 번째 프레임에서만 초기화 코드를 실행하기 위함.
C. 컴포넌트를 메모리에서 삭제하기 위함.
D. Input 단계보다 Render 단계를 먼저 실행하기 위함.
	 
	 
5.수정된 코드의 GameObject 소멸자에서 수행하는 작업 중 옳은 것은?

~GameObject() {
	for (int i = 0; i < (int)components.size(); i++) {
		delete components[i];
	}
}

A. 컴포넌트의 Start() 함수를 다시 호출함.
B. GameObject가 파괴될 때 자신이 가진 모든 컴포넌트의 메모리도 함께 해제함.
C. 전역 변수인 gameWorld를 비움.
D. vector의 요소만 비우고 실제 메모리는 그대로 둠.


---
#include <vector>
#include <string>

using namespace std;

class Component {
public:
class GameObject* pOwner = nullptr;
bool isStarted = false;

    virtual void Start()[(A)]
    virtual void Input() {}
    virtual void Update(float dt) = 0;
    virtual void Render() {}
    virtual ~Component() {}
};

class GameObject {
public:
    string name;
    vector<Component*> components;

    GameObject(string n) : name(n) {}
    ~GameObject() {
        // 인덱스 기반으로 컴포넌트 해제
        for (int i = 0; i < (int)components.size(); i++) {
            delete components[i];
        }
}

    void AddComponent(Component* pComp) {
pComp->pOwner = this;
components.push_back(pComp);
}
};

class GravityBody : public Component {
public:
    float y = 0.0f;
    float velocityY = 0.0f;

    void Start() override { y = 10.0f; velocityY = 0.0f; }
    void Update(float dt) override {
        velocityY += 9.8f * dt;
        [(B)]
}
    void Render() override {
        printf("Object [%s] Height: %.2f\n", pOwner->name.c_str(), y);
}
};

class GameLoop {
public:
    bool isRunning = true;
    vector<GameObject*> gameWorld;
    chrono::high_resolution_clock::time_point prevTime;

    void Initialize() {
        prevTime = chrono::high_resolution_clock::now();
}

    void Run() {
        while ([(C)]) {
            // 시간 계산
            chrono::high_resolution_clock::time_point currentTime = chrono::high_resolution_clock::now();
            chrono::duration<float> elapsed = currentTime - prevTime;
            float dt = elapsed.count();
prevTime = currentTime;

            // 1. Start 호출 체크 (지연 초기화)
            for (int i = 0; i < (int)gameWorld.size(); i++) {
                GameObject* go = gameWorld[i];
                for (int j = 0; j < (int)go->components.size(); j++) {
                    Component* c = go->components[j];
                    if (c->isStarted == false) {
                        c->Start();
                        c->isStarted = true;
}
}
}

            // 2. Input 단계
            for (int i = 0; i < (int)gameWorld.size(); i++) {
                GameObject* go = gameWorld[i];
                for (int j = 0; j < (int)go->components.size(); j++) {
                    go->components[j]->Input();
                }
            }

            // 3. Update 단계
            for (int i = 0; i < (int)gameWorld.size(); i++) {
                GameObject* go = gameWorld[i];
                for (int j = 0; j < (int)go->components.size(); j++) {
                    go->components[j]->Update(dt);
                }
            }

            // 4. Render 단계
system("cls");
            for (int i = 0; i < (int)gameWorld.size(); i++) {
                GameObject* go = gameWorld[i];
                for (int j = 0; j < (int)go->components.size(); j++) {
                    go->components[j]->Render();
}
}

            if (GetAsyncKeyState(VK_ESCAPE)) isRunning = false;
            this_thread::sleep_for(chrono::milliseconds(10));
}
}
};

int main() {
    GameLoop engine;
    engine.Initialize();

    GameObject* ball = new GameObject("Ball");
    ball->AddComponent(new GravityBody());
    engine.gameWorld.push_back(ball);

    engine.Run();

    // 메모리 정리 (시험 문제용으로 남겨둠)
    for (int i = 0; i < (int)engine.gameWorld.size(); i++) {
        delete engine.gameWorld[i];
    }

return 0;
}