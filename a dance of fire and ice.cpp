﻿#include <windows.h>
#include <windowsx.h>
#include <math.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define CIRCLE_RADIUS 30
#define BLOCK_WIDTH 100
#define BLOCK_HEIGHT 100
#define ANIMATION_SPEED 0.229375f // 애니메이션 속도 조절
#define SCREEN_SPEED 6.0f // 화면 이동 속도 조절

HBITMAP hBackgroundBitmap = NULL;
bool isMusicPlaying = false;
bool isGameStarted = false;

struct CircleState {
    float x, y;
    float targetY;
    float startY;
    float progress;
    bool isMovingToCenter;
    bool isWaitingAtCenter;
};

CircleState topCircle = { 400, CIRCLE_RADIUS, 300, CIRCLE_RADIUS, 0, true, false };
CircleState bottomCircle = { 400, 600 - CIRCLE_RADIUS, 300, 600 - CIRCLE_RADIUS, 0, true, false };
POINT center = { 400, 300 };
HWND hWnd;
HWND hStartButton; // 게임 시작 버튼
int currentBlock = 0;
float screenOffset = 0;

// Function declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void DrawGame(HDC hdc);
void UpdatePositions();
float EaseInOutQuad(float t);

void LoadBackgroundImage() {
    hBackgroundBitmap = (HBITMAP)LoadImage(NULL, L"bg.jpg", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
}

void PlayBackgroundMusic() {
    if (!isMusicPlaying) {
        PlaySound(L"./onrepeat_1.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
        isMusicPlaying = true;
    }
}

float EaseInOutQuad(float t) {
    return t < 0.5f ? 2.0f * t * t : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"AnimationWindowClass";

    if (!RegisterClassEx(&wcex)) return 0;

    LoadBackgroundImage();

    hWnd = CreateWindow(L"AnimationWindowClass", L"대기화면", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hWnd) return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (hBackgroundBitmap) DeleteObject(hBackgroundBitmap);
    return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        hStartButton = CreateWindow(
            L"BUTTON", L"게임 시작",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            350, 250, 100, 50,
            hWnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // 게임 시작 버튼이 눌렸을 때
            DestroyWindow(hStartButton); // 시작 버튼 제거
            SetTimer(hWnd, 1, 4, NULL); // 애니메이션 타이머 설정
            isGameStarted = true; // 게임 시작 상태로 변경
            SetWindowText(hWnd, L"리듬 게임"); // 윈도우 제목 변경

            PlayBackgroundMusic(); // **음악 재생**
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (isGameStarted) {
            DrawGame(hdc);
        }
        else {
            RECT clientRect;
            GetClientRect(hWnd, &clientRect);
            HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(30, 30, 30));
            FillRect(hdc, &clientRect, hBackgroundBrush);
            DeleteObject(hBackgroundBrush);
        }
        EndPaint(hWnd, &ps);
    }
                 break;
    case WM_TIMER:
        if (isGameStarted) {
            UpdatePositions();
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_KEYDOWN:
        if (isGameStarted && wParam == VK_SPACE) {
            PlayBackgroundMusic();
        }
        break;
    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void DrawGame(HDC hdc) {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // 더블 버퍼링을 위한 메모리 DC 생성
    HDC hMemDC = CreateCompatibleDC(hdc);
    HBITMAP hBufferBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBufferBitmap);

    // 배경 색상 지우기
    HBRUSH hBackgroundBrush = CreateSolidBrush(RGB(30, 30, 30));
    FillRect(hMemDC, &clientRect, hBackgroundBrush);
    DeleteObject(hBackgroundBrush);

    // 애니메이션 그리기
    if (hBackgroundBitmap != NULL) {
        HDC hBitmapDC = CreateCompatibleDC(hMemDC);
        HBITMAP hOldBackgroundBitmap = (HBITMAP)SelectObject(hBitmapDC, hBackgroundBitmap);

        BITMAP bitmap;
        GetObject(hBackgroundBitmap, sizeof(BITMAP), &bitmap);
        StretchBlt(hMemDC, 0, 0, width, height, hBitmapDC, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);

        SelectObject(hBitmapDC, hOldBackgroundBitmap);
        DeleteDC(hBitmapDC);
    }

    int numBlocks = width / BLOCK_WIDTH + 2;
    for (int i = 0; i < numBlocks; i++) {
        RECT blockRect = {
            (int)(i * BLOCK_WIDTH - (int)screenOffset % BLOCK_WIDTH),
            center.y - BLOCK_HEIGHT / 2,
            (int)((i + 1) * BLOCK_WIDTH - (int)screenOffset % BLOCK_WIDTH),
            center.y + BLOCK_HEIGHT / 2
        };

        HBRUSH hBlockBrush = CreateSolidBrush(RGB(60, 60, 60));
        FillRect(hMemDC, &blockRect, hBlockBrush);
        DeleteObject(hBlockBrush);

        HPEN hBlockPen = CreatePen(PS_SOLID, 2, RGB(60, 60, 600));
        HPEN hOldPen = (HPEN)SelectObject(hMemDC, hBlockPen);
        Rectangle(hMemDC, blockRect.left, blockRect.top, blockRect.right, blockRect.bottom);
        SelectObject(hMemDC, hOldPen);
        DeleteObject(hBlockPen);
    }

    HBRUSH hBrushTop = CreateSolidBrush(RGB(255, 0, 0));
    HBRUSH hOldBrushTop = (HBRUSH)SelectObject(hMemDC, hBrushTop);
    Ellipse(hMemDC,
        (int)(topCircle.x - CIRCLE_RADIUS),
        (int)(topCircle.y - CIRCLE_RADIUS),
        (int)(topCircle.x + CIRCLE_RADIUS),
        (int)(topCircle.y + CIRCLE_RADIUS));
    SelectObject(hMemDC, hOldBrushTop);
    DeleteObject(hBrushTop);

    HBRUSH hBrushBottom = CreateSolidBrush(RGB(0, 0, 255));
    HBRUSH hOldBrushBottom = (HBRUSH)SelectObject(hMemDC, hBrushBottom);
    Ellipse(hMemDC,
        (int)(bottomCircle.x - CIRCLE_RADIUS),
        (int)(bottomCircle.y - CIRCLE_RADIUS),
        (int)(bottomCircle.x + CIRCLE_RADIUS),
        (int)(bottomCircle.y + CIRCLE_RADIUS));
    SelectObject(hMemDC, hOldBrushBottom);
    DeleteObject(hBrushBottom);

    // 최종적으로 화면에 출력
    BitBlt(hdc, 0, 0, width, height, hMemDC, 0, 0, SRCCOPY);

    // 메모리 정리
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBufferBitmap);
    DeleteDC(hMemDC);
}


void UpdatePositions() {
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    int height = clientRect.bottom - clientRect.top;

    // 화면 이동 속도
    screenOffset += SCREEN_SPEED;

    // 원의 애니메이션 업데이트
    if (!topCircle.isWaitingAtCenter) {
        topCircle.progress += ANIMATION_SPEED;
        if (topCircle.progress >= 1.0f) {
            topCircle.progress = 0;
            topCircle.y = topCircle.targetY;
            if (topCircle.isMovingToCenter) {
                topCircle.isWaitingAtCenter = true;
                topCircle.isMovingToCenter = false;
            }
            else {
                topCircle.startY = CIRCLE_RADIUS;
                topCircle.targetY = center.y;
                topCircle.isMovingToCenter = true;
            }
        }
        else {
            float easedProgress = EaseInOutQuad(topCircle.progress);
            topCircle.y = topCircle.startY + (topCircle.targetY - topCircle.startY) * easedProgress;
        }
    }

    if (!bottomCircle.isWaitingAtCenter) {
        bottomCircle.progress += ANIMATION_SPEED;
        if (bottomCircle.progress >= 1.0f) {
            bottomCircle.progress = 0;
            bottomCircle.y = bottomCircle.targetY;
            if (bottomCircle.isMovingToCenter) {
                bottomCircle.isWaitingAtCenter = true;
                bottomCircle.isMovingToCenter = false;
            }
            else {
                bottomCircle.startY = height - CIRCLE_RADIUS;
                bottomCircle.targetY = center.y;
                bottomCircle.isMovingToCenter = true;
            }
        }
        else {
            float easedProgress = EaseInOutQuad(bottomCircle.progress);
            bottomCircle.y = bottomCircle.startY + (bottomCircle.targetY - bottomCircle.startY) * easedProgress;
        }
    }

    // 원들이 모두 중심에서 대기 중일 때 상태 전환
    if (topCircle.isWaitingAtCenter && bottomCircle.isWaitingAtCenter) {
        currentBlock++;
        topCircle.isWaitingAtCenter = false;
        bottomCircle.isWaitingAtCenter = false;
        topCircle.startY = center.y;
        topCircle.targetY = CIRCLE_RADIUS;
        topCircle.isMovingToCenter = false;
        bottomCircle.startY = center.y;
        bottomCircle.targetY = clientRect.bottom - CIRCLE_RADIUS;
        bottomCircle.isMovingToCenter = false;
    }

    // 원의 x 좌표 고정
    topCircle.x = bottomCircle.x = 400;
}