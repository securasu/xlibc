#ifndef _XARRAY_H_
#define _XARRAY_H_

#include <stddef.h>

/*
 * sparse array (or "radix tree"). suitable for a series of continuous indexs.
 *
 * +-+-+---+---+---+---+
 * | 0 | 1 | 2 |...|N-1|
 * |   |nil|   |...|nil| (this is a block)
 * +-+-+---+-+-+---+---+
 *   |       |
 *   |       +------------------+
 *   |                          |
 *   v                          v
 * +-+-+---+---+---+---+      +-+-+---+---+---+---+
 * | 0 | 1 | 2 |...|N-1|      | 0 | 1 | 2 |...|N-1|
 * |nil|   |nil|...|nil|      |nil|   |nil|...|nil|
 * +-+-+---+---+---+---+      +---+-+-+---+---+---+
 *       |                          |
 *       v                          v
 *     +-+--+                     +-+--+
 *     |Node|                     |Node|
 *     |Data|                     |Data| (this is a node)
 *     +----+                     +----+
 *     Index 0                   Index 2N+1
 */

/* cache can decrease memory allocation. node (or block) will
 * be put into cache when it being erased, and next insertion
 * will pop one node (or block) from cache. define 'XARRAY_ENABLE_CACHE=1'
 * to enable it. */
#ifndef XARRAY_ENABLE_CACHE
#define XARRAY_ENABLE_CACHE 0
#endif

#ifndef XARRAY_BITS
#define XARRAY_BITS         6   // [1, 8]
#endif

#ifndef XARRAY_INDEX_BITS
#define XARRAY_INDEX_BITS   32  // 16, 32, 64
#endif

typedef struct xarray       xarray_t;
typedef struct xarray_block xarray_block_t;
typedef struct xarray_node  xarray_node_t;
typedef struct xarray_node* xarray_iter_t;

typedef void (*xarray_destroy_cb)(void*);

#if XARRAY_INDEX_BITS == 16
typedef unsigned short xuint;
#elif XARRAY_INDEX_BITS == 64
typedef unsigned long long xuint;
#else // 32
typedef unsigned int xuint;
#endif

#define XARRAY_BLOCK_SIZE   (1 << XARRAY_BITS)

struct xarray_node
{
    xarray_block_t* block;
    xuint           index;
    // char data[0];
};

struct xarray_block
{
    xarray_block_t* parent_block;
    unsigned char   parent_pos;
    unsigned char   shift;
    unsigned char   used;   /* how many values current block had used. */
    void*           values[XARRAY_BLOCK_SIZE];
};

struct xarray
{
    size_t              blocks;     /* how many block had allocated in xarray. */
                                    /* current just for DEBUG. */
    size_t              values;     /* how many value had allocated in xarray. */
                                    /* current just for DEBUG. */
    size_t              val_size;
    xarray_destroy_cb   destroy_cb; /* called when value is removed. */
#if XARRAY_ENABLE_CACHE
    xarray_node_t*      nod_cache;  /* cache nodes. */
    xarray_block_t*     blk_cache;  /* cache blocks. */
#endif
    xarray_block_t      root;       /* root block. */
};

/* initialize a 'xarray_t'. */
xarray_t* xarray_init(xarray_t* array, size_t val_size, xarray_destroy_cb cb);
/* destroy a 'xarray_t' which has called 'xarray_init'. */
void xarray_destroy(xarray_t* array);

/* allocate memory and initialize a 'xarray_t'. */
xarray_t* xarray_new(size_t val_size, xarray_destroy_cb cb);
/* release memory for a 'xarray_t' which 'xarray_new' returns. */
void xarray_free(xarray_t* array);

#if XARRAY_ENABLE_CACHE
/* free all cache nodes in a 'xarray_t'. */
void xarray_node_cache_free(xarray_t* array);
/* free all cache blocks in a 'xarray_t'. */
void xarray_block_cache_free(xarray_t* array);
#endif

/* return an iterator to the end. */
#define xarray_end(array)   NULL

/* return an iterator to the beginning. */
xarray_iter_t xarray_begin(xarray_t* array);
/* return the next iterator of 'iter'. */
xarray_iter_t xarray_iter_next(xarray_iter_t iter);

/* return if 'iter' is valid. */
#define xarray_iter_valid(iter)     ((iter) != NULL)
/* return the index of 'iter', 'iter' MUST be valid. */
#define xarray_iter_index(iter)     ((iter)->index)
/* return a poiniter pointed to the value at 'iter', 'iter' MUST be valid. */
#define xarray_iter_value(iter)     ((void*)((iter) + 1))
/* return an iterator of an value value. */
#define xarray_value_iter(pvalue)   ((xarray_iter_t)(pvalue) - 1)

/* set value at 'index', 'pvalue' can be 'NULL' (means set it later,
 * just allocate memory). if 'index' has already been set, it will be
 * unset (call 'destroy_cb') first. return an iterator to the value
 * at 'index'. return 'NULL' if out of memory. */
xarray_iter_t xarray_set(xarray_t* array, xuint index, const void* pvalue);
/* get value at 'index'. return an iterator to the value at index,
 * return 'NULL' if value has not been set. */
xarray_iter_t xarray_get(xarray_t* array, xuint index);
/* remove value at 'index'. */
void xarray_unset(xarray_t* array, xuint index);
/* clear all values (no cache). */
void xarray_clear(xarray_t* array);

/* get value at 'index'. return a pointer pointed to the value at index,
 * return 'XARRAY_INVALID_VALUE' if value has not been set.
 * the return value can call 'xarray_value_iter' to get it's iterator. */
#define xarray_get_value(array, index) \
                xarray_iter_value(xarray_get(array, index))
/* set value at 'index', 'pvalue' can be 'NULL' (means set it later,
 * just allocate memory). if 'index' has already been set, it will be
 * unset (call 'destroy_cb') first. return a pointer pointed to the
 * value at 'index'. return 'XARRAY_INVALID_VALUE' if out of memory. */
#define xarray_set_value(array, index, pvalue) \
                xarray_iter_value(xarray_set(array, index, pvalue))

#define XARRAY_INVALID_VALUE  xarray_iter_value((xarray_iter_t)0)

#endif // _XARRAY_H_