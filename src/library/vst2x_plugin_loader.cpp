/**
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

#include "library/vst2x_plugin_loader.h"

#include "logging.h"

namespace sushi {
namespace vst2 {

MIND_GET_LOGGER;

VstIntPtr VSTCALLBACK host_callback(AEffect* effect,
                                    VstInt32 opcode, VstInt32 index,
                                    VstIntPtr value, void* ptr, float opt)
{
    VstIntPtr result = 0;

    MIND_LOG_DEBUG("PLUG> HostCallback (opcode {})\n index = {}, value = {}, ptr = {}, opt = {}\n", opcode, index, FromVstPtr<void> (value), ptr, opt);

    switch (opcode)
    {
    case audioMasterVersion :
        result = kVstVersion;
        break;
    default:
        break;
    }

    return result;

}

// TODO: this is POSIX specific and the Linux-way to do it.
// Works with Mac OS X as well, but can only load VSTs compiled in a POSIX way.

LibraryHandle PluginLoader::get_library_handle_for_plugin(const std::string &plugin_absolute_path)
{
    void *libraryHandle = dlopen(plugin_absolute_path.c_str(), RTLD_NOW | RTLD_LOCAL);

    if (libraryHandle == NULL)
    {
        MIND_LOG_ERROR("Could not open library, {}", dlerror());
        return NULL;
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

    if (entryPoint.entryPointVoidPtr == NULL)
    {
        entryPoint.entryPointVoidPtr = dlsym(library_handle, "main");
        if (entryPoint.entryPointVoidPtr == NULL)
        {
              MIND_LOG_ERROR("Couldn't get a pointer to plugin's main()");
              return NULL;
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
        MIND_LOG_WARNING("Could not safely close plugin, possible resource leak");
    }
}

} // namespace vst2
} // namespace sushi

