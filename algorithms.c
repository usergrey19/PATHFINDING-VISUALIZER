#include "visualizer.h"
#include <time.h>

// 4 Cardinal directions (Up, Down, Left, Right)
static const int dRow[] = {-1, 1, 0, 0};
static const int dCol[] = {0, 0, -1, 1};

static int manhattan(Node* a, Node* b) {
    return abs(a->r - b->r) + abs(a->c - b->c);
}

static void reconstructPath(SearchInstance* inst) {
    inst->pathCost = 0;
    inst->pathSteps = 0;
    Node* curr = inst->target->parent;
    inst->pathCost += inst->target->weight;

    while (curr != NULL && curr != inst->start) {
        curr->type = PATH;
        inst->pathCost += curr->weight;
        inst->pathSteps++;
        curr = curr->parent;
    }
}

void initSearchInstance(SearchInstance* inst, Node baseGrid[ROWS][COLS], int sr, int sc, int tr, int tc, AlgoType algo) {
    inst->algo = algo;
    inst->visitedCount = 0;
    inst->pathCost = 0;
    inst->pathSteps = 0;
    inst->elapsedMs = 0.0;
    inst->isFinished = false;

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            inst->grid[r][c] = baseGrid[r][c];
            inst->grid[r][c].g_cost = 999999;
            inst->grid[r][c].h_cost = 0;
            inst->grid[r][c].f_cost = 999999;
            inst->grid[r][c].visited = false;
            inst->grid[r][c].in_frontier = false;
            inst->grid[r][c].parent = NULL;

            if (inst->grid[r][c].base_type == WALL) {
                inst->grid[r][c].type = WALL;
            } else if (inst->grid[r][c].base_type == WEIGHT) {
                inst->grid[r][c].type = WEIGHT;
                inst->grid[r][c].weight = 5;
            } else {
                inst->grid[r][c].type = EMPTY;
                inst->grid[r][c].weight = 1;
            }
        }
    }

    inst->start = &inst->grid[sr][sc];
    inst->start->type = START;
    inst->start->weight = 1;

    inst->target = &inst->grid[tr][tc];
    inst->target->type = TARGET;
    inst->target->weight = 1;

    initMinHeap(&inst->pq);
    initQueue(&inst->bfsQueue);
    initStack(&inst->dfsStack);

    inst->start->g_cost = 0;
    inst->start->h_cost = manhattan(inst->start, inst->target);
    inst->start->f_cost = inst->start->g_cost + inst->start->h_cost;

    if (algo == ALGO_BFS) {
        inst->start->visited = true;
        enqueue(&inst->bfsQueue, inst->start);
    } else if (algo == ALGO_DFS) {
        pushStack(&inst->dfsStack, inst->start);
    } else if (algo == ALGO_DIJKSTRA) {
        pushMinHeap(&inst->pq, inst->start, 0);
    } else if (algo == ALGO_ASTAR) {
        pushMinHeap(&inst->pq, inst->start, inst->start->f_cost);
    }
}

bool stepSearchInstance(SearchInstance* inst) {
    if (inst->isFinished) return false;

    Node* curr = NULL;

    if (inst->algo == ALGO_BFS) {
        if (isQueueEmpty(&inst->bfsQueue)) {
            inst->isFinished = true;
            return false;
        }
        curr = dequeue(&inst->bfsQueue);
    } else if (inst->algo == ALGO_DFS) {
        while (!isStackEmpty(&inst->dfsStack)) {
            Node* candidate = popStack(&inst->dfsStack);
            if (!candidate->visited) {
                curr = candidate;
                curr->visited = true;
                break;
            }
        }
        if (curr == NULL) {
            inst->isFinished = true;
            return false;
        }
    } else {
        while (!isHeapEmpty(&inst->pq)) {
            Node* candidate = popMinHeap(&inst->pq);
            if (!candidate->visited) {
                curr = candidate;
                curr->visited = true;
                break;
            }
        }
        if (curr == NULL) {
            inst->isFinished = true;
            return false;
        }
    }

    inst->visitedCount++;

    if (curr == inst->target) {
        reconstructPath(inst);
        inst->isFinished = true;
        return false;
    }

    if (curr != inst->start && curr != inst->target) {
        curr->type = VISITED;
    }

    for (int i = 0; i < 4; i++) {
        int nr = curr->r + dRow[i];
        int nc = curr->c + dCol[i];

        if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {
            Node* neighbor = &inst->grid[nr][nc];
            if (neighbor->type == WALL || neighbor->visited) continue;

            if (inst->algo == ALGO_BFS) {
                neighbor->visited = true;
                neighbor->parent = curr;
                if (neighbor != inst->target) neighbor->type = FRONTIER;
                enqueue(&inst->bfsQueue, neighbor);
            } else if (inst->algo == ALGO_DFS) {
                neighbor->parent = curr;
                if (neighbor != inst->target) neighbor->type = FRONTIER;
                pushStack(&inst->dfsStack, neighbor);
            } else if (inst->algo == ALGO_DIJKSTRA || inst->algo == ALGO_ASTAR) {
                int stepCost = neighbor->weight;
                int newG = curr->g_cost + stepCost;
                if (newG < neighbor->g_cost) {
                    neighbor->g_cost = newG;
                    neighbor->h_cost = (inst->algo == ALGO_ASTAR) ? manhattan(neighbor, inst->target) : 0;
                    neighbor->f_cost = neighbor->g_cost + neighbor->h_cost;
                    neighbor->parent = curr;
                    if (neighbor != inst->target) neighbor->type = FRONTIER;
                    pushMinHeap(&inst->pq, neighbor, neighbor->f_cost);
                }
            }
        }
    }
    return true;
}

// ================= RECURSIVE BACKTRACKING MAZE GENERATOR =================
static void carvePassage(Node grid[ROWS][COLS], int r, int c) {
    int dirs[4] = {0, 1, 2, 3};
    for (int i = 3; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = dirs[i];
        dirs[i] = dirs[j];
        dirs[j] = tmp;
    }

    for (int i = 0; i < 4; i++) {
        int dr = dRow[dirs[i]] * 2;
        int dc = dCol[dirs[i]] * 2;
        int nr = r + dr;
        int nc = c + dc;

        if (nr > 0 && nr < ROWS - 1 && nc > 0 && nc < COLS - 1) {
            if (grid[nr][nc].base_type == WALL) {
                grid[r + dRow[dirs[i]]][c + dCol[dirs[i]]].base_type = EMPTY;
                grid[r + dRow[dirs[i]]][c + dCol[dirs[i]]].type = EMPTY;
                grid[nr][nc].base_type = EMPTY;
                grid[nr][nc].type = EMPTY;
                carvePassage(grid, nr, nc);
            }
        }
    }
}

void generateMaze(Node grid[ROWS][COLS], Node* start, Node* target) {
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS; c++) {
            grid[r][c].base_type = WALL;
            grid[r][c].type = WALL;
            grid[r][c].weight = 1;
        }
    }

    grid[1][1].base_type = EMPTY;
    grid[1][1].type = EMPTY;
    carvePassage(grid, 1, 1);

    for (int r = 2; r < ROWS - 2; r++) {
        for (int c = 2; c < COLS - 2; c++) {
            if (grid[r][c].base_type == WALL && (rand() % 100) < 12) {
                grid[r][c].base_type = EMPTY;
                grid[r][c].type = EMPTY;
            }
        }
    }

    int sr = start->r, sc = start->c;
    int tr = target->r, tc = target->c;
    grid[sr][sc].base_type = EMPTY;
    grid[sr][sc].type = START;
    grid[tr][tc].base_type = EMPTY;
    grid[tr][tc].type = TARGET;

    for (int i = 0; i < 4; i++) {
        int nsr = sr + dRow[i], nsc = sc + dCol[i];
        if (nsr > 0 && nsr < ROWS - 1 && nsc > 0 && nsc < COLS - 1) {
            grid[nsr][nsc].base_type = EMPTY;
            if (&grid[nsr][nsc] != target) grid[nsr][nsc].type = EMPTY;
        }
        int ntr = tr + dRow[i], ntc = tc + dCol[i];
        if (ntr > 0 && ntr < ROWS - 1 && ntc > 0 && ntc < COLS - 1) {
            grid[ntr][ntc].base_type = EMPTY;
            if (&grid[ntr][ntc] != start) grid[ntr][ntc].type = EMPTY;
        }
    }
}