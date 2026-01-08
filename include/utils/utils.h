
#ifndef MYUTILS_H
#define MYUTILS_H

#include <stddef.h>
#include <stdint.h>

#define DA_APPEND(x, it)\
    do \
    {\
        if (x.size >= x.capacity) \
        {\
            if (x.capacity == 0) x.capacity = 50; \
            else x.capacity *= 2;\
            x.arr = realloc(x.arr, x.capacity*sizeof(*x.arr));\
        }\
        x.arr[x.size++] = it;\
    } while (0)


typedef struct 
{
    uint8_t *arr;
    size_t size;
    size_t capacity;
} CharBuffer;

typedef struct 
{
    uint8_t *arr;
    size_t size;
    size_t capacity;
} uint8Buffer;

#endif 