#ifndef VISUALIZER_H
#define VISUALIZER_H

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#define ROWS 25
#define COLS 43
#define CELL_SIZE 25
#define HEADER_HEIGHT 85
#define SCREEN_WIDTH (COLS * CELL_SIZE)
#define SCREEN_HEIGHT (ROWS * CELL_SIZE + HEADER_HEIGHT)
#define MAX_NODES (ROWS * COLS)

typedef enum {
    EMPTY,
    WALL,
    WEIGHT,   // Mud tile (Cost = 5)
    START,
    TARGET,
    FRONTIER,
    VISITED,
    PATH
} CellType;

typedef enum {
    ALGO_BFS = 0,
    ALGO_DFS = 1,
    ALGO_DIJKSTRA = 2,
    ALGO_ASTAR = 3
} AlgoType;

typedef struct Node {
    int r, c;
    int weight;      // 1 for normal, 5 for mud
    int g_cost;
    int h_cost;
    int f_cost;
    bool visited;
    bool in_frontier;
    CellType type;
    CellType base_type; // Preserves WEIGHT when visited/path
    struct Node* parent;
} Node;

// --- Data Structure 1: Binary Min-Heap (Dijkstra & A*) ---
typedef struct {
    Node* node;
    int priority;
} HeapElement;

typedef struct {
    HeapElement elements[MAX_NODES * 4];
    int size;
} MinHeap;

// --- Data Structure 2: Circular FIFO Queue (BFS) ---
typedef struct {
    Node* items[MAX_NODES];
    int front;
    int rear;
    int count;
} Queue;

// --- Data Structure 3: LIFO Stack (DFS) ---
typedef struct {
    Node* items[MAX_NODES * 4];
    int top;
} Stack;

// Encapsulated Instance so all 4 Algorithms can run simultaneously!
typedef struct {
    AlgoType algo;
    Node grid[ROWS][COLS];
    Node* start;
    Node* target;
    MinHeap pq;
    Queue bfsQueue;
    Stack dfsStack;
    int visitedCount;
    int pathCost;
    int pathSteps;
    double elapsedMs;
    bool isFinished;
} SearchInstance;

// Data Structure Operations (datastructures.c)
void initMinHeap(MinHeap* h);
void pushMinHeap(MinHeap* h, Node* node, int priority);
Node* popMinHeap(MinHeap* h);
bool isHeapEmpty(MinHeap* h);

void initQueue(Queue* q);
void enqueue(Queue* q, Node* node);
Node* dequeue(Queue* q);
bool isQueueEmpty(Queue* q);

void initStack(Stack* s);
void pushStack(Stack* s, Node* node);
Node* popStack(Stack* s);
bool isStackEmpty(Stack* s);

// Algorithm & Maze Operations (algorithms.c)
void initSearchInstance(SearchInstance* inst, Node baseGrid[ROWS][COLS], int sr, int sc, int tr, int tc, AlgoType algo);
bool stepSearchInstance(SearchInstance* inst);
void generateMaze(Node grid[ROWS][COLS], Node* start, Node* target);

#endif