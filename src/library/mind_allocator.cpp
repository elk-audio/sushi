//
// Created by gustav on 10/13/16.
//
#include <cstddef>
#include <cstdlib>
#include <EABase/eabase.h>
#include <new>

#include "mind_allocator.h"

void* operator new[](size_t size, const char* /*pName*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return malloc(size);
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/, const char* /*pName*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return malloc(size);
}