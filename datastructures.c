#include "visualizer.h"

// ================= BINARY MIN-HEAP =================
void initMinHeap(MinHeap* h) {
    h->size = 0;
}

static void swapHeapElements(HeapElement* a, HeapElement* b) {
    HeapElement temp = *a;
    *a = *b;
    *b = temp;
}

void pushMinHeap(MinHeap* h, Node* node, int priority) {
    if (h->size >= MAX_NODES * 4) return;
    int i = h->size++;
    h->elements[i].node = node;
    h->elements[i].priority = priority;

    while (i != 0 && h->elements[(i - 1) / 2].priority > h->elements[i].priority) {
        swapHeapElements(&h->elements[i], &h->elements[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

Node* popMinHeap(MinHeap* h) {
    if (h->size <= 0) return NULL;
    if (h->size == 1) return h->elements[--h->size].node;

    Node* root = h->elements[0].node;
    h->elements[0] = h->elements[--h->size];

    int i = 0;
    while (2 * i + 1 < h->size) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = left;

        if (right < h->size && h->elements[right].priority < h->elements[left].priority) {
            smallest = right;
        }
        if (h->elements[i].priority <= h->elements[smallest].priority) break;

        swapHeapElements(&h->elements[i], &h->elements[smallest]);
        i = smallest;
    }
    return root;
}

bool isHeapEmpty(MinHeap* h) {
    return h->size == 0;
}

// ================= CIRCULAR FIFO QUEUE =================
void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->count = 0;
}

void enqueue(Queue* q, Node* node) {
    if (q->count >= MAX_NODES) return;
    q->rear = (q->rear + 1) % MAX_NODES;
    q->items[q->rear] = node;
    q->count++;
}

Node* dequeue(Queue* q) {
    if (q->count <= 0) return NULL;
    Node* node = q->items[q->front];
    q->front = (q->front + 1) % MAX_NODES;
    q->count--;
    return node;
}

bool isQueueEmpty(Queue* q) {
    return q->count == 0;
}

// ================= LIFO STACK =================
void initStack(Stack* s) {
    s->top = -1;
}

void pushStack(Stack* s, Node* node) {
    if (s->top >= (MAX_NODES * 4) - 1) return;
    s->items[++(s->top)] = node;
}

Node* popStack(Stack* s) {
    if (s->top < 0) return NULL;
    return s->items[(s->top)--];
}

bool isStackEmpty(Stack* s) {
    return s->top < 0;
}