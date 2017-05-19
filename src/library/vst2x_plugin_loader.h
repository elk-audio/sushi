/**
 * @brief Utilities for loading plugins stored in dynamic libraries
 * @copyright 2017 MIND Music Labs AB, Stockholm
 *
 * Parts taken and/or adapted from:
 * MrsWatson - https://github.com/teragonaudio/MrsWatson
 *
 * Original copyright notice with BSD license:
 * Copyright (c) 2013 Teragon Audio. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SUSHI_VST2X_PLUGIN_LOADER_H
#define SUSHI_VST2X_PLUGIN_LOADER_H

#include <string>
#include <cstdlib>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#define VST_FORCE_DEPRECATED 0
#include "aeffectx.h"
#pragma GCC diagnostic pop

#include <dlfcn.h>

namespace sushi {
namespace vst2 {

typedef void* LibraryHandle;

// TODO:
//      this class is stateless atm (basically a namespace),
//      but it should probably grow into the access point to plugins stored in the
//      system, with features like directory scanning, caching, easier handling
//      of library handles, etc.

class PluginLoader
{
public:
    static LibraryHandle get_library_handle_for_plugin(const std::string& plugin_absolute_path);

    static AEffect* load_plugin(LibraryHandle library_handle);

    static void close_library_handle(LibraryHandle library_handle);

};

} // namespace vst2
} // namespace sushi

#endif //SUSHI_VST2X_PLUGIN_LOADER_H
