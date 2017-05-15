/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <pmm.h>
#include <list.h>
#include <string.h>
#include <buddy.h>

free_area_t free_area;

#define free_list (free_area.free_list)
#define nr_free (free_area.nr_free)

// Global block
static size_t buddy_physical_size;
static size_t buddy_virtual_size;
static size_t buddy_segment_size;
static size_t buddy_alloc_size;
static size_t *buddy_segment;
static struct Page *buddy_physical;
static struct Page *buddy_alloc;

#define MIN(a,b) ((a)<(b)?(a):(b))

// Bitwise operate
#define UINT32_SHR_OR(a,n)    ((a) | ((a)>>(n)))
#define UINT32_MASK(a)        (UINT32_SHR_OR(UINT32_SHR_OR(UINT32_SHR_OR(UINT32_SHR_OR(a,1),2),4),8),16)
#define UINT32_REMAINDER(a)   ((a) & (UINT32_MASK(a)>>1))
#define UINT32_ROUND_UP(a)   (UINT32_REMAINDER(a)?(((a)-UINT32_REMAINDER(a))<<1):(a))
#define UINT32_ROUND_DWON(a) (UINT32_REMAINDER(a)?((a)-UINT32_REMAINDER(a)):(a))

// Buddy operate
#define BUDDY_ROOT            (1)
#define BUDDY_LEFT(a)         ((a)<<1)
#define BUDDY_RIGHT(a)        (((a)<<1)+1)
#define BUDDY_PARENT(a)       ((a)>>1)
#define BUDDY_LENGTH(a)       (buddy_virtual_size/UINT32_ROUND_DOWN(a))         // 虚拟分配空间第几个block的大小
#define BUDDY_BEGIN(a)        (UINT32_REMAINDER(a)*BUDDY_LENGTH(a))             // 虚拟分配空间第几个block的起始位置
#define BUDDY_END(a)          ((UINT32_REMAINDER(a)+1)*BUDDY_LENGTH(a))         // 虚拟分配空间第几个block的结束位置
#define BUDDY_BLOCK(a,b)      (buddy_virtual_size/((b)-(a))+(a)/((b)-(a)))      
#define BUDDY_EMPTY(a)        (buddy_segment[(a)] == BUDDY_LENGTH(a))

static void
buddy_init_size(size_t n) {
    assert(n > 1);
    buddy_physical_size = n;
    if (n < 512) {
        buddy_virtual_size = UINT32_ROUND_UP(n - 1);
        buddy_segment_size = 1;
    } else {
        buddy_virtual_size = UINT32_ROUND_DWON(n);
        buddy_segment_size = buddy_virtual_size * sizeof(size_t)*2 / PGSIZE;
        if (n > buddy_virtual_size + (buddy_segment_size << 1)) {
            buddy_virtual_size <<= 1;
            buddy_segment_size <<= 1;
        }
    }
    buddy_alloc_size = MIN(buddy_virtual_size, buddy_physical_size - buddy_segment_size);
}

static void
buddy_init_segment(struct Page *base) {
    // Init address
    buddy_physical = base;
    buddy_segment = KADDR(page2pa(base));
    buddy_alloc = base + buddy_segment_size;
    memset(buddy_segment, 0, buddy_segment_size*PGSIZE);
    // Init segment
    nr_free += buddy_alloc_size;
    size_t block = BUDDY_ROOT;
    size_t alloc_size = buddy_alloc_size;
    size_t virtual_size = buddy_virtual_size;
    buddy_segment[block] = alloc_size;
    while (alloc_size > 0 && alloc_size < virtual_size) {
        virtual_size >>= 1;
        if (alloc_size > virtual_size) {
            // Add left to free list
            struct Page *page = &buddy_alloc[BUDDY_BEGIN(block)];
            page->property = virtual_size;
            list_add(&(free_list), &(page->page_link));
            buddy_segment[BUDDY_LEFT(block)] = virtual_size;
            // Switch to right
            alloc_size -= virtual_size;
            buddy_segment[BUDDY_RIGHT(block)] = alloc_size;
            block = BUDDY_RIGHT(block);
        } else {
            // Switch to left
            buddy_segment[BUDDY_LEFT(block)] = alloc_size;
            buddy_segment[BUDDY_RIGHT(block)] = 0;
            block = BUDDY_LEFT(block);
        }
    }
    if (alloc_size > 0) {
        struct Page *page = &buddy_alloc[BUDDY_BEGIN(block)];
        page->property = alloc_size;
        list_add(&(free_list), &(page->page_link));
    }
}

static void
buddy_init(void) {
    list_init(&free_list);
    nr_free = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0);
    // Init pages
    for (struct Page *p = base; p < base + n; p++) {
        assert(PageReserved(p));
        p->flags = p->property = 0;
    }
    // Init size
    buddy_init_size(n);
    // Init segment
    buddy_init_segment(base);
}

static struct Page*
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    struct Page *page;
    size_t block = BUDDY_ROOT;
    size_t length = UINT32_ROUND_UP(n);
    // Find block
    while (length <= buddy_segment[block] && length < BUDDY_LENGTH(block)) {
        size_t left = BUDDY_LEFT(block);
        size_t right = BUDDY_RIGHT(block);
        if (BUDDY_EMPTY(block)) {
            size_t begin = BUDDY_BEGIN(block);
            size_t end = BUDDY_END(block);
        }

    }

}