/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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

/**
 * @brief Utilities for loading plugins stored in dynamic libraries
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
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
#include <cstdlib>
#ifdef __APPLE__
#include <Corefoundation/Corefoundation.h>
#endif

#include "vst2x_plugin_loader.h"
#include "vst2x_host_callback.h"

#include "logging.h"

namespace sushi {
namespace vst2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("vst2");

// TODO: this is POSIX specific and the Linux-way to do it.
// Works with Mac OS X as well, but can only load VSTs compiled in a POSIX way.

#if defined(__linux__)

LibraryHandle PluginLoader::get_library_handle_for_plugin(const std::string &plugin_absolute_path)
{
    if (plugin_absolute_path.empty())
    {
        SUSHI_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
                        // program, which can cause an infinite loop.
    }
    void *libraryHandle = dlopen(plugin_absolute_path.c_str(), RTLD_NOW | RTLD_LOCAL);

    if (libraryHandle == nullptr)
    {
        SUSHI_LOG_ERROR("Could not open library, {}", dlerror());
        return nullptr;
    }

    return libraryHandle;
}

AEffect* PluginLoader::load_plugin(LibraryHandle library_handle)
{
    // Somewhat cheap hack to avoid a tricky compiler warning. Casting from void*
    // to a proper function pointer will cause GCC to warn that "ISO C++ forbids
    // casting between pointer-to-function and pointer-to-object". Here, we
    // represent both types in a union and use the correct one in the given
    // context, thus avoiding the need to cast anything.  See also:
    // http://stackoverflow.com/a/2742234/14302
    union
    {
        plugin_entry_proc entryPointFuncPtr;
        void *entryPointVoidPtr;
    } entryPoint;

    entryPoint.entryPointVoidPtr = dlsym(library_handle, "VSTPluginMain");

    if (entryPoint.entryPointVoidPtr == nullptr)
    {
        entryPoint.entryPointVoidPtr = dlsym(library_handle, "main");
        if (entryPoint.entryPointVoidPtr == nullptr)
        {
              SUSHI_LOG_ERROR("Couldn't get a pointer to plugin's main()");
              return nullptr;
        }
    }

    plugin_entry_proc mainEntryPoint = entryPoint.entryPointFuncPtr;
    AEffect *plugin = mainEntryPoint(host_callback);
    return plugin;
}

void PluginLoader::close_library_handle(LibraryHandle library_handle)
{
    if (dlclose(library_handle) != 0)
    {
        SUSHI_LOG_WARNING("Could not safely close plugin, possible resource leak");
    }
}

#elif defined(__APPLE__)
LibraryHandle PluginLoader::get_library_handle_for_plugin(const std::string &plugin_absolute_path)
{
    if (plugin_absolute_path.empty())
    {
        SUSHI_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
                        // program, which can cause an infinite loop.
    }
    CFURLRef bundleURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                                  (const UInt8*) plugin_absolute_path.c_str(),
                                                                  plugin_absolute_path.size(),
                                                                  true );

    CFBundleRef bundleHandle = CFBundleCreate(kCFAllocatorDefault, bundleURL);

    if (bundleHandle == nullptr)
    {
        SUSHI_LOG_ERROR("Could not open bundle");
        return nullptr;
    }

    return (LibraryHandle)bundleHandle;
}

AEffect* PluginLoader::load_plugin(LibraryHandle library_handle)
{
    // Somewhat cheap hack to avoid a tricky compiler warning. Casting from void*
    // to a proper function pointer will cause GCC to warn that "ISO C++ forbids
    // casting between pointer-to-function and pointer-to-object". Here, we
    // represent both types in a union and use the correct one in the given
    // context, thus avoiding the need to cast anything.  See also:
    // http://stackoverflow.com/a/2742234/14302
    union
    {
        plugin_entry_proc entryPointFuncPtr;
        void *entryPointVoidPtr;
    } entryPoint;

    entryPoint.entryPointVoidPtr = CFBundleGetFunctionPointerForName ((CFBundleRef)library_handle, CFSTR("main_macho"));

    if (entryPoint.entryPointVoidPtr == nullptr)
    {
        entryPoint.entryPointVoidPtr = CFBundleGetFunctionPointerForName ((CFBundleRef)library_handle, CFSTR("VSTPluginMain"));
        if (entryPoint.entryPointVoidPtr == nullptr)
        {
            entryPoint.entryPointVoidPtr = CFBundleGetFunctionPointerForName ((CFBundleRef)library_handle, CFSTR("main"));
            if (entryPoint.entryPointVoidPtr == nullptr)
            {
              SUSHI_LOG_ERROR("Couldn't get a pointer to plugin's main()");
              return nullptr;
            }
        }
    }

    plugin_entry_proc mainEntryPoint = entryPoint.entryPointFuncPtr;
    AEffect *plugin = mainEntryPoint(host_callback);
    return plugin;
}

void PluginLoader::close_library_handle(LibraryHandle library_handle)
{
    CFRelease((CFBundleRef)library_handle);
    if (library_handle != nullptr)
    {
        SUSHI_LOG_WARNING("Could not safely close plugin, possible resource leak");
    }
}
#endif

} // namespace vst2
} // namespace sushi

