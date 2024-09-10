# Minimum required CMake version updated to 3.5 for better Android support
cmake_minimum_required(VERSION 3.5)

# Windows specific configurations excluding unnecessary Android checks
if (WIN32 AND NOT CYGWIN AND NOT (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore"))
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
endif()

project(XLua)

# iOS specific flags
if (IOS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
endif()

# Options for custom features
option(UINT_ESPECIALLY "Using custom ulong" OFF)
option(USING_LUAJIT "Using LuaJIT" OFF)
option(GC64 "Using GC64" OFF)
option(LUAC_COMPATIBLE_FORMAT "Compatible format" OFF)

# Locate XLua project directory
find_path(XLUA_PROJECT_DIR NAMES SConstruct PATHS ${CMAKE_SOURCE_DIR} NO_DEFAULT_PATH)
MARK_AS_ADVANCED(XLUA_PROJECT_DIR)

# Set Lua version if not provided
if (NOT LUA_VERSION)
    set(LUA_VERSION "5.3.5")
endif()

# Lua source paths
set(LUA_SRC_PATH lua-${LUA_VERSION}/src)
set(LUAJIT_SRC_PATH luajit-2.1.0b3/src)

# Configure LuaJIT-specific settings
if (USING_LUAJIT)
    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${LUAJIT_SRC_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${THIRDPART_INC}
    )
    set(LUA_CORE "")
    set_property(SOURCE xlua.c APPEND PROPERTY COMPILE_DEFINITIONS USING_LUAJIT)
else ()
    set(LUA_IDSIZE 120 CACHE STRING "Maximum size for the description of the source.")
    configure_file(${LUA_SRC_PATH}/luaconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/luaconf.h)

    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${LUA_SRC_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${THIRDPART_INC}
    )

    aux_source_directory(${LUA_SRC_PATH} LUA_CORE)
    list(REMOVE_ITEM LUA_CORE ${LUA_SRC_PATH}/lua.c ${LUA_SRC_PATH}/luac.c)
    set(LUA_LIB "")
endif()

# LuaSocket source files
set(LUA_SOCKET
    luasocket/auxiliar.c
    luasocket/buffer.c
    luasocket/except.c
    luasocket/inet.c
    luasocket/io.c
    luasocket/luasocket.c
    luasocket/mime.c
    luasocket/options.c
    luasocket/select.c
    luasocket/tcp.c
    luasocket/timeout.c
    luasocket/udp.c
)

# Platform specific socket files
if (WIN32 AND NOT CYGWIN)
    list(APPEND LUA_SOCKET luasocket/wsocket.c)
else()
    list(APPEND LUA_SOCKET luasocket/usocket.c)
endif()

# XLua core source files
set(XLUA_CORE
    i64lib.c
    xlua.c
)

if (NOT USING_LUAJIT)
    list(APPEND XLUA_CORE memory_leak_checker.c)
endif()

# Macro for organizing source files into groups (for IDEs like Visual Studio)
macro(source_group_by_dir proj_dir source_files)
    if(MSVC OR APPLE)
        get_filename_component(sgbd_cur_dir ${proj_dir} ABSOLUTE)
        foreach(sgbd_file ${${source_files}})
            get_filename_component(sgbd_abs_file ${sgbd_file} ABSOLUTE)
            file(RELATIVE_PATH sgbd_fpath ${sgbd_cur_dir} ${sgbd_abs_file})
            string(REGEX REPLACE "\(.*\)/.*" \\1 sgbd_group_name ${sgbd_fpath})
            string(COMPARE EQUAL ${sgbd_fpath} ${sgbd_group_name} sgbd_nogroup)
            string(REPLACE "/" "\\" sgbd_group_name ${sgbd_group_name})
            if(sgbd_nogroup)
                set(sgbd_group_name "\\")
            endif()
            source_group(${sgbd_group_name} FILES ${sgbd_file})
        endforeach()
    endif()
endmacro()

# Organize source files by directory
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_SOCKET)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_CORE)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_LIB)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} XLUA_CORE)

# Add platform-specific configurations
if (APPLE)
    if (IOS)
        ADD_DEFINITIONS(-DLUA_USE_IOS)
        set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
        add_library(xlua STATIC ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
        set_xcode_property(xlua IPHONEOS_DEPLOYMENT_TARGET "7.0" "all")
    else()
        ADD_DEFINITIONS(-DLUA_USE_MACOSX)
        if (BUILD_SILICON)
            set(CMAKE_OSX_ARCHITECTURES arm64)
            add_library(xlua SHARED ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
        else()
            set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD_64_BIT)")
            add_library(xlua MODULE ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
            set_target_properties(xlua PROPERTIES BUNDLE TRUE)
        endif()
    endif()
elseif(ANDROID)
    # Android specific configuration
    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${LUA_SRC_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${THIRDPART_INC}
    )
    add_library(xlua SHARED ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
    target_link_libraries(xlua log android ${THIRDPART_LIB})
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Switch")
    add_library(xlua STATIC ${LUA_CORE} ${LUA_LIB} ${XLUA_CORE} ${THIRDPART_SRC})
    target_compile_options(xlua PRIVATE -m64 -mcpu=cortex-a57+fp+simd+crypto+crc -fno-common -fno-short-enums -ffunction-sections -fdata-sections -fPIC -fms-extensions)
else()
    add_library(xlua SHARED ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
endif()

# Windows specific linking
if (WIN32 AND NOT CYGWIN)
    target_compile_definitions(xlua PRIVATE LUA_BUILD_AS_DLL)
    target_link_libraries(xlua ws2_32 ${THIRDPART_LIB})
else()
    if (USING_LUAJIT AND NOT APPLE)
        target_link_libraries(xlua ${CMAKE_SOURCE_DIR}/${LUAJIT_SRC_PATH}/libluajit.a m ${THIRDPART_LIB})
    else()
        target_link_libraries(xlua m ${THIRDPART_LIB})
    endif()
endif()

# Additional compile definitions for specific options
if (GC64 AND USING_LUAJIT)
    target_compile_definitions(xlua PRIVATE LUAJIT_ENABLE_GC64)
endif()

if (LUAC_COMPATIBLE_FORMAT AND NOT USING_LUAJIT)
    target_compile_definitions(xlua PRIVATE LUAC_COMPATIBLE_FORMAT)
endif()

# LuaSocket definitions for cross-platform compatibility
set_property(
    SOURCE ${LUA_SOCKET}
    APPEND
    PROPERTY COMPILE_DEFINITIONS LUA_COMPAT_APIINTCASTS LUA_LIB _CRT_SECURE_NO_WARNINGS _WINSOCK_DEPRECATED_NO_WARNINGS
)

# Custom ulong option definition
if (UINT_ESPECIALLY)
    ADD_DEFINITIONS(-DUINT_ESPECIALLY)
endif()
