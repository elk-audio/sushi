/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
   @file symap.h API for Symap, a basic symbol map (string interner).

   Particularly useful for implementing LV2 URI mapping.

   @see <a href="http://lv2plug.in/ns/ext/urid">LV2 URID</a>
*/

// This file has been copied from the Jalv LV2 plugin host example - no refactoring has been carried out

#ifndef SYMAP_H
#define SYMAP_H

#include <cstdint>

namespace lv2_host {

struct SymapImpl;

typedef struct SymapImpl Symap;

/**
Create a new symbol map.
*/
Symap* symap_new(void);

/**
Free a symbol map.
*/
void symap_free(Symap* map);

/**
Map a string to a symbol ID if it is already mapped, otherwise return 0.
*/
uint32_t symap_try_map(Symap* map, const char* sym);

/**
Map a string to a symbol ID.

Note that 0 is never a valid symbol ID.
*/
uint32_t symap_map(Symap* map, const char* sym);

/**
Unmap a symbol ID back to a symbol, or NULL if no such ID exists.

Note that 0 is never a valid symbol ID.
*/
const char* symap_unmap(Symap* map, uint32_t id);

}

#endif /* SYMAP_H */