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
* @brief Warning Suppressor
* @Copyright 2017-2023 Elk Audio AB, Stockholm
*/

/**
 * This allows suppressing specific warnings for certain (third-party) includes,
 * in a manner that is compiler-independent.
 * It should work with Clang, GCC, and MSVC (MSVC is untested).
 *
 * Usage example#include "warning_suppressor.h":
 *
 * #include "warning_suppressor.h"
 *
 * ELK_PUSH_WARNING
 * ELK_DISABLE_WARNING (WARN_SHADOW_FIELD)
 * ELK_DISABLE_WARNING (WARN_EXTRA_SEMI)
 * ELK_DISABLE_WARNING (WARN_ZERO_AS_NULL_POINTER_CONSTANT)
 * ELK_DISABLE_WARNING (WARN_SIGN_CONVERSION)
 * #include <JuceHeader.h>
 *
 * #include "anyrpc/anyrpc.h"
 * #include "anyrpc/value.h"
 * ELK_POP_WARNING
 */

#pragma once

/**
 * From here:
 * https://stackoverflow.com/questions/45762357/how-to-concatenate-strings-in-the-arguments-of-pragma/45783809#45783809
 */
#define DO_PRAGMA_(x) _Pragma (#x)
#define DO_PRAGMA(x) DO_PRAGMA_(x)

/**
 * Combining this:
 * https://software.codidact.com/posts/280528
 * With this:
 * https://stackoverflow.com/questions/28166565/detect-gcc-as-opposed-to-msvc-clang-with-macro
 * And this:
 * https://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html
 */
#if defined(__clang__)
#define ELK_PUSH_WARNING DO_PRAGMA(clang diagnostic push)
#elif defined(__GNUC__) || defined(__GNUG__)
#define ELK_PUSH_WARNING DO_PRAGMA(GCC diagnostic push)
#elif defined(_MSC_VER)
#define ELK_PUSH_WARNING DO_PRAGMA(warnings(push))
#endif

#if defined(__clang__)
#define ELK_POP_WARNING DO_PRAGMA(clang diagnostic pop)
#elif defined(__GNUC__) || defined(__GNUG__)
#define ELK_POP_WARNING DO_PRAGMA(GCC diagnostic pop)
#elif defined(_MSC_VER)
#define ELK_POP_WARNING DO_PRAGMA(warnings(pop))
#endif

#if defined(__clang__)
#define ELK_DISABLE_WARNING(WARNING_NAME) DO_PRAGMA(clang diagnostic ignored WARNING_NAME##_CLANG)
#elif defined(__GNUC__) || defined(__GNUG__)
#define ELK_DISABLE_WARNING(WARNING_NAME) DO_PRAGMA(GCC diagnostic ignored WARNING_NAME##_GCC)
#elif defined(_MSC_VER)
#define ELK_DISABLE_WARNING(WARNING_NAME) DO_PRAGMA(warnings(disable :  WARNING_NAME##_MSC)
#endif

#define WARN_UNINITIALIZED_CLANG "-Wuninitialized"
#define WARN_UNINITIALIZED_GCC "-Wuninitialized"

#define WARN_UNUSED_FUNCTION_CLANG "-Wunused-function"
#define WARN_UNUSED_FUNCTION_GCC "-Wunused-function"

#define WARN_SHADOW_FIELD_CLANG "-Wshadow-field"
#define WARN_SHADOW_FIELD_GCC "-Wshadow-field"

#define WARN_EXTRA_SEMI_CLANG "-Wextra-semi"
#define WARN_EXTRA_SEMI_GCC "-Wextra-semi"

#define WARN_EXTRA_CLANG "-Wextra"
#define WARN_EXTRA_GCC "-Wextra"

#define WARN_ZERO_AS_NULL_POINTER_CONSTANT_CLANG "-Wzero-as-null-pointer-constant"
#define WARN_ZERO_AS_NULL_POINTER_CONSTANT_GCC "-Wzero-as-null-pointer-constant"

#define WARN_SIGN_CONVERSION_CLANG "-Wsign-conversion"
#define WARN_SIGN_CONVERSION_GCC "-Wsign-conversion"

#define WARN_CONDITIONAL_UNINITIALIZED_CLANG "-Wconditional-uninitialized"
#define WARN_CONDITIONAL_UNINITIALIZED_GCC "-Wconditional-uninitialized"

#define WARN_KEYWORD_MACRO_CLANG "-Wkeyword-macro"
#define WARN_KEYWORD_MACRO_GCC "-Wkeyword-macro"

#define WARN_UNUSED_PARAMETER_CLANG "-Wunused-parameter"
#define WARN_UNUSED_PARAMETER_GCC "-Wunused-parameter"

#define WARN_TYPE_LIMITS_CLANG "-Wtype-limits"
#define WARN_TYPE_LIMITS_GCC "-Wtype-limits"