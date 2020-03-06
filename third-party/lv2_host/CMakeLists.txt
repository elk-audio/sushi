add_library(lv2_host STATIC src/lv2_evbuf.cpp
        src/lv2_symap.cpp)

target_include_directories(lv2_host PUBLIC include)

target_compile_features(lv2_host PRIVATE cxx_std_17)
target_compile_options(lv2_host PRIVATE -Wall -Wextra -Wno-psabi -fno-rtti -ffast-math)