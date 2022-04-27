#include "memalloc.h"

#ifndef MEM_CUSTOM_TOTAL_SIZE
#define MEM_CUSTOM_TOTAL_SIZE 100000
#endif

int mem_custom_initialized = 0;
char mem_custom[MEM_CUSTOM_TOTAL_SIZE] = { 0 };
struct mem_block* mem_freeList = (struct mem_block*)mem_custom;

void mem_initialize() {
    memset(mem_custom, 0, MEM_CUSTOM_TOTAL_SIZE);
    mem_freeList->size = MEM_CUSTOM_TOTAL_SIZE - sizeof(struct mem_block);
    mem_freeList->free = 1;
    mem_freeList->next = NULL;
    mem_custom_initialized = 1;
}

void mem_split(struct mem_block* fitting_slot, size_t size) {
    struct mem_block* new = (void*)((void*)fitting_slot + size + sizeof(struct mem_block));
    new->size = (fitting_slot->size) - size - sizeof(struct mem_block);
    new->free = 1;
    new->next = fitting_slot->next;
    fitting_slot->size = size;
    fitting_slot->free = 0;
    fitting_slot->next = new;
}

void* AllocMemBlock(size_t noOfBytes) {
    struct mem_block* curr, * prev;
    void* result;
    if (!mem_freeList->size || mem_custom_initialized != 1) {
        mem_initialize();
    }
    curr = mem_freeList;
    while ((((curr->size) < noOfBytes) || ((curr->free) == 0)) && (curr->next != NULL)) {
        prev = curr;
        curr = curr->next;
    }
    if ((curr->size) == noOfBytes) {
        curr->free = 0;
        result = (void*)(++curr);
        return result;
    }
    else if ((curr->size) > (noOfBytes + sizeof(struct mem_block))) {
        mem_split(curr, noOfBytes);
        result = (void*)(++curr);
        return result;
    }
    else {
        result = NULL;
        return result;
    }
}

void mem_merge() {
    struct mem_block* curr, * prev;
    curr = mem_freeList;
    while ((curr->next) != NULL) {
        if ((curr->free) && (curr->next->free)) {
            curr->size += (curr->next->size) + sizeof(struct mem_block);
            curr->next = curr->next->next;
        }
        prev = curr;
        curr = curr->next;
    }
}

void FreeMemBlock(void* ptr) {
    if (((void*)mem_custom <= ptr) && (ptr <= (void*)(mem_custom + MEM_CUSTOM_TOTAL_SIZE))) {
        struct mem_block* curr = ptr;
        --curr;
        curr->free = 1;
        mem_merge();
    }
}