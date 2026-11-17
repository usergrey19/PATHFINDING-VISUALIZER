#include <windows.h>
#include "visualizer.h"

static Node baseGrid[ROWS][COLS];
static Node* startNode = NULL;
static Node* targetNode = NULL;

static SearchInstance instances[4]; // 0: BFS, 1: DFS, 2: Dijkstra, 3: A*
static AlgoType activeSingleAlgo = ALGO_ASTAR;
static bool compareAllMode = false; // Press [5] to toggle 4-Way Split Screen!
static bool isRunning = false;
static bool drawMudMode = false;    // Press [W] to toggle Wall vs Mud brush
static int stepsPerFrame = 2;

static bool draggingStart = false;
static bool draggingTarget = false;
static bool leftMouseDown = false;
static bool rightMouseDown = false;

static LARGE_INTEGER perfFreq;

const char* getAlgoTitle(AlgoType a) {
    switch (a) {
        case ALGO_BFS:      return "1. BFS (Circular FIFO Queue)";
        case ALGO_DFS:      return "2. DFS (LIFO Stack)";
        case ALGO_DIJKSTRA: return "3. Dijkstra (Binary Min-Heap)";
        case ALGO_ASTAR:    return "4. A* Search (Min-Heap + Manhattan)";
        default:            return "";
    }
}

void syncInstancesFromBase(void) {
    int sr = startNode->r, sc = startNode->c;
    int tr = targetNode->r, tc = targetNode->c;
    for (int i = 0; i < 4; i++) {
        initSearchInstance(&instances[i], baseGrid, sr, sc, tr, tc, (AlgoType)i);
    }
}

void resetBaseGrid(bool keepObstacles) {
    isRunning = false;
    int sr = startNode ? startNode->r : ROWS / 2;
    int sc = startNode ? startNode->c : 5;
    int tr = targetNode ? targetNode->r : ROWS / 2;
    int tc = targetNode ? targetNode->c : COLS - 6;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            CellType prevBase = keepObstacles ? baseGrid[r][c].base_type : EMPTY;
            baseGrid[r][c].r = r;
            baseGrid[r][c].c = c;
            baseGrid[r][c].base_type = prevBase;
            baseGrid[r][c].type = prevBase;
            baseGrid[r][c].weight = (prevBase == WEIGHT) ? 5 : 1;
        }
    }

    baseGrid[sr][sc].base_type = EMPTY;
    baseGrid[sr][sc].type = START;
    startNode = &baseGrid[sr][sc];

    baseGrid[tr][tc].base_type = EMPTY;
    baseGrid[tr][tc].type = TARGET;
    targetNode = &baseGrid[tr][tc];

    syncInstancesFromBase();
}

void handleMouseAt(int x, int y) {
    if (isRunning || compareAllMode || y < HEADER_HEIGHT) return;
    int c = x / CELL_SIZE;
    int r = (y - HEADER_HEIGHT) / CELL_SIZE;

    if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
        Node* hovered = &baseGrid[r][c];
        if (leftMouseDown) {
            if (draggingStart && hovered != targetNode && hovered->base_type != WALL) {
                startNode->type = startNode->base_type;
                hovered->type = START;
                startNode = hovered;
            } else if (draggingTarget && hovered != startNode && hovered->base_type != WALL) {
                targetNode->type = targetNode->base_type;
                hovered->type = TARGET;
                targetNode = hovered;
            } else if (!draggingStart && !draggingTarget && hovered != startNode && hovered != targetNode) {
                hovered->base_type = drawMudMode ? WEIGHT : WALL;
                hovered->type = hovered->base_type;
                hovered->weight = drawMudMode ? 5 : 1;
            }
        } else if (rightMouseDown) {
            if (hovered != startNode && hovered != targetNode) {
                hovered->base_type = EMPTY;
                hovered->type = EMPTY;
                hovered->weight = 1;
            }
        }
        syncInstancesFromBase();
    }
}

COLORREF getCellColor(Node* n) {
    switch (n->type) {
        case WALL:     return RGB(148, 163, 184); // Slate Wall
        case WEIGHT:   return RGB(146, 64, 14);   // Brown Mud (Cost 5)
        case START:    return RGB(34, 197, 94);   // Green Start
        case TARGET:   return RGB(239, 68, 68);   // Red Target
        case FRONTIER: return RGB(56, 189, 248);  // Cyan Frontier
        case VISITED:
            return (n->base_type == WEIGHT) ? RGB(180, 83, 9) : RGB(99, 102, 241);
        case PATH:     return RGB(250, 204, 21);  // Gold Shortest Path
        case EMPTY:
        default:       return RGB(22, 32, 56);
    }
}

void drawSingleInstance(HDC memDC, SearchInstance* inst) {
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            HBRUSH cellBrush = CreateSolidBrush(getCellColor(&inst->grid[r][c]));
            RECT cellRect = {
                c * CELL_SIZE,
                r * CELL_SIZE + HEADER_HEIGHT,
                (c + 1) * CELL_SIZE - 1,
                (r + 1) * CELL_SIZE + HEADER_HEIGHT - 1
            };
            FillRect(memDC, &cellRect, cellBrush);
            DeleteObject(cellBrush);
        }
    }
}

void drawComparisonQuadrants(HDC memDC) {
    int quadW = SCREEN_WIDTH / 2;
    int quadH = (SCREEN_HEIGHT - HEADER_HEIGHT) / 2;
    int miniCell = 11;

    HFONT titleFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                 DEFAULT_PITCH | FF_SWISS, "Segoe UI");
    SelectObject(memDC, titleFont);

    for (int q = 0; q < 4; q++) {
        int qx = (q % 2) * quadW + 18;
        int qy = HEADER_HEIGHT + (q / 2) * quadH + 6;
        SearchInstance* inst = &instances[q];

        char stats[200];
        sprintf(stats, "%s  |  Visited: %d  |  Cost: %d  |  %.2f ms",
                getAlgoTitle((AlgoType)q), inst->visitedCount, inst->pathCost, inst->elapsedMs);
        SetTextColor(memDC, inst->isFinished && inst->pathCost > 0 ? RGB(250, 204, 21) : RGB(56, 189, 248));
        TextOutA(memDC, qx, qy, stats, strlen(stats));

        int gridTop = qy + 22;
        for (int r = 0; r < ROWS; r++) {
            for (int c = 0; c < COLS; c++) {
                HBRUSH b = CreateSolidBrush(getCellColor(&inst->grid[r][c]));
                RECT rc = {
                    qx + c * miniCell,
                    gridTop + r * miniCell,
                    qx + (c + 1) * miniCell - 1,
                    gridTop + (r + 1) * miniCell - 1
                };
                FillRect(memDC, &rc, b);
                DeleteObject(b);
            }
        }
    }
    DeleteObject(titleFont);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            int x = LOWORD(lParam), y = HIWORD(lParam);
            leftMouseDown = true;
            if (!isRunning && !compareAllMode && y >= HEADER_HEIGHT) {
                int c = x / CELL_SIZE, r = (y - HEADER_HEIGHT) / CELL_SIZE;
                if (r >= 0 && r < ROWS && c >= 0 && c < COLS) {
                    if (&baseGrid[r][c] == startNode) draggingStart = true;
                    else if (&baseGrid[r][c] == targetNode) draggingTarget = true;
                }
            }
            handleMouseAt(x, y);
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        }
        case WM_LBUTTONUP:
            leftMouseDown = false;
            draggingStart = false;
            draggingTarget = false;
            return 0;

        case WM_RBUTTONDOWN:
            rightMouseDown = true;
            handleMouseAt(LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_RBUTTONUP:
            rightMouseDown = false;
            return 0;

        case WM_MOUSEMOVE:
            if (leftMouseDown || rightMouseDown) {
                handleMouseAt(LOWORD(lParam), HIWORD(lParam));
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_KEYDOWN:
            if (wParam == '1') { compareAllMode = false; activeSingleAlgo = ALGO_BFS; syncInstancesFromBase(); }
            if (wParam == '2') { compareAllMode = false; activeSingleAlgo = ALGO_DFS; syncInstancesFromBase(); }
            if (wParam == '3') { compareAllMode = false; activeSingleAlgo = ALGO_DIJKSTRA; syncInstancesFromBase(); }
            if (wParam == '4') { compareAllMode = false; activeSingleAlgo = ALGO_ASTAR; syncInstancesFromBase(); }
            if (wParam == '5') { compareAllMode = !compareAllMode; isRunning = false; syncInstancesFromBase(); }

            if (wParam == VK_SPACE && !isRunning) {
                syncInstancesFromBase();
                isRunning = true;
            }
            if (wParam == 'M' && !isRunning) {
                generateMaze(baseGrid, startNode, targetNode);
                syncInstancesFromBase();
            }
            if (wParam == 'W') drawMudMode = !drawMudMode;
            if (wParam == VK_OEM_PLUS || wParam == VK_ADD) {
                if (stepsPerFrame < 20) stepsPerFrame++;
            }
            if (wParam == VK_OEM_MINUS || wParam == VK_SUBTRACT) {
                if (stepsPerFrame > 1) stepsPerFrame--;
            }
            if (wParam == 'R') syncInstancesFromBase();
            if (wParam == 'C') resetBaseGrid(false);

            InvalidateRect(hwnd, NULL, FALSE);
            return 0;

        case WM_TIMER:
            if (isRunning) {
                bool anyActive = false;
                for (int s = 0; s < stepsPerFrame; s++) {
                    if (compareAllMode) {
                        for (int q = 0; q < 4; q++) {
                            if (!instances[q].isFinished) {
                                LARGE_INTEGER t1, t2;
                                QueryPerformanceCounter(&t1);
                                bool cont = stepSearchInstance(&instances[q]);
                                QueryPerformanceCounter(&t2);
                                instances[q].elapsedMs += (double)(t2.QuadPart - t1.QuadPart) * 1000.0 / perfFreq.QuadPart;
                                if (cont) anyActive = true;
                            }
                        }
                    } else {
                        SearchInstance* inst = &instances[activeSingleAlgo];
                        if (!inst->isFinished) {
                            LARGE_INTEGER t1, t2;
                            QueryPerformanceCounter(&t1);
                            bool cont = stepSearchInstance(inst);
                            QueryPerformanceCounter(&t2);
                            inst->elapsedMs += (double)(t2.QuadPart - t1.QuadPart) * 1000.0 / perfFreq.QuadPart;
                            if (cont) anyActive = true;
                        }
                    }
                }
                if (!anyActive) isRunning = false;
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            HDC memDC = CreateCompatibleDC(hdc);
            HBITMAP memBM = CreateCompatibleBitmap(hdc, SCREEN_WIDTH, SCREEN_HEIGHT);
            SelectObject(memDC, memBM);

            HBRUSH bgBrush = CreateSolidBrush(RGB(15, 23, 42));
            RECT fullRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            FillRect(memDC, &fullRect, bgBrush);
            DeleteObject(bgBrush);

            HBRUSH headerBrush = CreateSolidBrush(RGB(30, 41, 59));
            RECT headerRect = {0, 0, SCREEN_WIDTH, HEADER_HEIGHT};
            FillRect(memDC, &headerRect, headerBrush);
            DeleteObject(headerBrush);

            SetBkMode(memDC, TRANSPARENT);
            HFONT hFont = CreateFont(17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                     OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                     DEFAULT_PITCH | FF_SWISS, "Segoe UI");
            SelectObject(memDC, hFont);

            char line1[256];
            if (compareAllMode) {
                sprintf(line1, "MODE: 4-WAY SIMULTANEOUS ALGORITHM COMPARISON  |  Speed [+/-]: %dx", stepsPerFrame);
                SetTextColor(memDC, RGB(250, 204, 21));
            } else {
                SearchInstance* cur = &instances[activeSingleAlgo];
                sprintf(line1, "%s  |  Visited: %d  |  Path Cost: %d  |  Time: %.2f ms  |  Brush [W]: %s  |  Speed: %dx",
                        getAlgoTitle(activeSingleAlgo), cur->visitedCount, cur->pathCost, cur->elapsedMs,
                        drawMudMode ? "MUD (Cost 5)" : "WALL", stepsPerFrame);
                SetTextColor(memDC, RGB(56, 189, 248));
            }
            TextOutA(memDC, 14, 12, line1, strlen(line1));

            const char* line2 = "[1-4] Single Algo   [5] Compare All 4!   [SPACE] Run   [M] Gen Maze   [W] Wall/Mud Brush   [+/-] Speed   [R] Reset   [C] Clear";
            SetTextColor(memDC, RGB(203, 213, 225));
            TextOutA(memDC, 14, 46, line2, strlen(line2));
            DeleteObject(hFont);

            if (compareAllMode) {
                drawComparisonQuadrants(memDC);
            } else {
                drawSingleInstance(memDC, &instances[activeSingleAlgo]);
            }

            BitBlt(hdc, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, memDC, 0, 0, SRCCOPY);
            DeleteObject(memBM);
            DeleteDC(memDC);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    QueryPerformanceFrequency(&perfFreq);

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "PathfinderFinalClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);

    RECT winRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    AdjustWindowRect(&winRect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE);

    CreateWindowExA(
        0, "PathfinderFinalClass", "DSA Pathfinding Visualizer - Final Phase (100% Complete - 4-Way Comparison)",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        winRect.right - winRect.left, winRect.bottom - winRect.top,
        NULL, NULL, hInstance, NULL
    );

    resetBaseGrid(false);
    SetTimer(FindWindowA("PathfinderFinalClass", NULL), 1, 16, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}