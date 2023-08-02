/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

// This file has been copied from the Jalv LV2 plugin host example
// - no refactoring other than aesthetic has been carried out.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "lv2_host/lv2_symap.h"

/**
  @file symap.c Implementation of Symap, a basic symbol map (string interner).

  This implementation is primitive, but has some desirable qualities: good
  (O(lg(n)) lookup performance for already-mapped symbols, minimal space
  overhead, extremely fast (O(1)) reverse mapping (ID to string), simple code,
  no dependencies.

  The tradeoff is that mapping new symbols may be quite slow.  In other words,
  this implementation is ideal for use cases with a relatively limited set of
  symbols, or where most symbols are mapped early.  It will not fare so well
  with very dynamic sets of symbols.  For that, you're better off with a
  tree-based implementation (and the associated space cost, especially if you
  need reverse mapping).
*/

namespace lv2_host {

struct SymapImpl
{
    /**
       Unsorted array of strings, such that the symbol for ID i is found
       at symbols[i - 1].
    */
    char **symbols;

    /**
       Array of IDs, sorted by corresponding string in `symbols`.
    */
    u_int32_t *index;

    /**
       Number of symbols (number of items in `symbols` and `index`).
    */
    u_int32_t size;
};

Symap* symap_new(void)
{
    auto map = (Symap*) malloc(sizeof(Symap));
    map->symbols = NULL;
    map->index = NULL;
    map->size = 0;
    return map;
}

void symap_free(Symap *map)
{
    if (!map)
    {
        return;
    }

    for (uint32_t i = 0; i < map->size; ++i)
    {
        free(map->symbols[i]);
    }

    free(map->symbols);
    free(map->index);
    free(map);
}

static char* symap_strdup(const char *str)
{
    const size_t len = strlen(str);
    char *copy = (char *) malloc(len + 1);
    memcpy(copy, str, len + 1);
    return copy;
}

/**
Return the index into map->index (not the ID) corresponding to `sym`,
or the index where a new entry for `sym` should be inserted.
*/
static uint32_t symap_search(const Symap *map, const char *sym, bool *exact)
{
    *exact = false;

    if (map->size == 0)
    {
        return 0;  // Empty map, insert at 0
    }
    else if (strcmp(map->symbols[map->index[map->size - 1] - 1], sym) < 0)
    {
        return map->size;  // Greater than last element, append
    }

    uint32_t lower = 0;
    uint32_t upper = map->size - 1;
    uint32_t i = upper;
    int cmp;

    while (upper >= lower)
    {
        i = lower + ((upper - lower) / 2);
        cmp = strcmp(map->symbols[map->index[i] - 1], sym);

        if (cmp == 0)
        {
            *exact = true;
            return i;
        }
        else if (cmp > 0)
        {
            if (i == 0)
            {
                break;  // Avoid underflow
            }

            upper = i - 1;
        }
        else
        {
            lower = ++i;
        }
    }

    assert(!*exact || strcmp(map->symbols[map->index[i] - 1], sym) > 0);
    return i;
}

uint32_t symap_try_map(Symap *map, const char *sym)
{
    bool exact;
    const uint32_t index = symap_search(map, sym, &exact);

    if (exact)
    {
        assert(!strcmp(map->symbols[map->index[index]], sym));
        return map->index[index];
    }

    return 0;
}

uint32_t symap_map(Symap *map, const char *sym)
{
    bool exact;
    const uint32_t index = symap_search(map, sym, &exact);

    if (exact)
    {
        assert(!strcmp(map->symbols[map->index[index] - 1], sym));
        return map->index[index];
    }

    const uint32_t id = ++map->size;
    char *const str = symap_strdup(sym);

    /* Append new symbol to symbols array */
    map->symbols = (char **) realloc(map->symbols, map->size * sizeof(str));
    map->symbols[id - 1] = str;

    /* Insert new index element into sorted index */
    map->index = (uint32_t *) realloc(map->index, map->size * sizeof(uint32_t));

    if (index < map->size - 1)
    {
        memmove(map->index + index + 1,
                map->index + index,
                (map->size - index - 1) * sizeof(uint32_t));
    }

    map->index[index] = id;

    return id;
}

const char* symap_unmap(Symap *map, uint32_t id)
{
    if (id == 0)
    {
        return nullptr;
    }
    else if (id <= map->size)
    {
        return map->symbols[id - 1];
    }

    return nullptr;
}

}
