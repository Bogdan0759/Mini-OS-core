#define HEAP_START 0x100000
#define HEAP_SIZE 0x10000

static uint8_t heap[HEAP_SIZE];
static uint32_t heap_index = 0;

void *malloc(size_t size) {
    if (heap_index + size > HEAP_SIZE) {
        return NULL;
    }
    void *ptr = &heap[heap_index];
    heap_index += size;
    return ptr;
}

void free(void *ptr) {
    
}