//
// Created by joshua on 08.08.16.
//

#ifndef KNX_NEW_H
#define KNX_NEW_H

#include <stdlib.h>

void * operator new(size_t size)
{
    return malloc(size);
}

/*void operator delete(void * ptr)
{
    free(ptr);
}*/

void * operator new[](size_t size)
{
    return malloc(size);
}

void operator delete[](void * ptr)
{
    free(ptr);
}

#endif //KNX_NEW_H
