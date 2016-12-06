//
// Created by gustav on 10/13/16.
//

#ifndef SUSHI_MIND_ALLOCATOR_H
#define SUSHI_MIND_ALLOCATOR_H

/*
 * Default allocators for EASTL
 */
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags,
                     const char* file, int line);

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName,
                     int flags, unsigned debugFlags, const char* file, int line);


#endif //SUSHI_MIND_ALLOCATOR_H
