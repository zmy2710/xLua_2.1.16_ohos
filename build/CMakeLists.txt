cmake_minimum_required(VERSION 3.5)

# Windows specific settings
if (WIN32 AND NOT CYGWIN AND NOT (CMAKE_SYSTEM_NAME STREQUAL "WindowsStore") AND NOT ANDROID)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT" CACHE STRING "")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /MTd" CACHE STRING "")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT" CACHE STRING "")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd" CACHE STRING "")
endif()

project(XLua)

# iOS specific settings
if (IOS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fembed-bitcode")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fembed-bitcode")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum OS X deployment version")
endif()

# Android specific settings
if (ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    add_definitions(-DANDROID)
endif()

# Custom options
option(UINT_ESPECIALLY "using custom ulong" OFF)
option(USING_LUAJIT "using luajit" OFF)
option(GC64 "using gc64" OFF)

# LUAC compatible format option
if(IOS)
    option(LUAC_COMPATIBLE_FORMAT "compatible format" OFF)
else()
    option(LUAC_COMPATIBLE_FORMAT "compatible format" ON)
endif()

# Define Lua version
add_definitions(-DLUA_VERSION_NUM=503)

# Find Lua project directory
find_path(XLUA_PROJECT_DIR NAMES SConstruct
    PATHS 
    ${CMAKE_SOURCE_DIR}
    NO_DEFAULT_PATH
)
MARK_AS_ADVANCED(XLUA_PROJECT_DIR)

# Define Lua version if not specified
if(NOT LUA_VERSION)
    set(LUA_VERSION "5.3.5")
endif()

# Define Lua paths
set(LUA_SRC_PATH lua-${LUA_VERSION}/src)
set(LUAJIT_SRC_PATH luajit-2.1.0b3/src)

# LuaJIT specific settings
if(USING_LUAJIT)
    include_directories(
        ${CMAKE_SOURCE_DIR}
        ${LUAJIT_SRC_PATH}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${THIRDPART_INC}
    )
    set(LUA_CORE)
    set_property(SOURCE xlua.c APPEND PROPERTY COMPILE_DEFINITIONS USING_LUAJIT)
    set(LUA_LIB)
else()
    # Lua 5.3 specific settings
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
    set(LUA_LIB)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Third-party libraries inclusion
set(MEMSTREAM_SRC luamemstream/memstream.c)
set_property(SOURCE ${MEMSTREAM_SRC} APPEND PROPERTY COMPILE_DEFINITIONS LUA_LIB)
set(THIRDPART_SRC ${THIRDPART_SRC} ${MEMSTREAM_SRC})

set(PROTOBUF_SRC lua-protobuf/pb.c)
set_property(SOURCE ${PROTOBUF_SRC} APPEND PROPERTY COMPILE_DEFINITIONS LUA_LIB)
list(APPEND THIRDPART_INC lua-rapidjson/include)
set(THIRDPART_SRC ${THIRDPART_SRC} ${PROTOBUF_SRC})

# RapidJSON integration
set(RAPIDJSON_SRC
    lua-rapidjson/source/rapidjson.cpp
    lua-rapidjson/source/Document.cpp
    lua-rapidjson/source/Schema.cpp
    lua-rapidjson/source/values.cpp
)
set_property(SOURCE ${RAPIDJSON_SRC} APPEND PROPERTY COMPILE_DEFINITIONS LUA_LIB)
list(APPEND THIRDPART_INC lua-rapidjson/include)
set(THIRDPART_SRC ${THIRDPART_SRC} ${RAPIDJSON_SRC})
include_directories(lua-rapidjson/include)

# LuaSocket integration
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
if(WIN32 AND NOT CYGWIN)
    list(APPEND LUA_SOCKET luasocket/wsocket.c)
else()
    list(APPEND LUA_SOCKET luasocket/usocket.c)
endif()

# XLua core files
set(XLUA_CORE
    i64lib.c
    xlua.c
)

if(NOT USING_LUAJIT)
    list(APPEND XLUA_CORE memory_leak_checker.c)
endif()

# Source grouping for better organization
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

source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_SOCKET)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} LUA_CORE)
source_group_by_dir(${CMAKE_CURRENT_SOURCE_DIR} XLUA_CORE)

# Platform-specific build settings
if(APPLE)
    if(IOS)
        set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
        add_library(xlua STATIC ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
        set_xcode_property(xlua IPHONEOS_DEPLOYMENT_TARGET "7.0" "all")
    else()
        ADD_DEFINITIONS(-DLUA_USE_MACOSX)
        add_library(xlua MODULE ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
        set_target_properties(xlua PROPERTIES BUNDLE TRUE)
    endif()
elseif(ANDROID)
    add_library(xlua SHARED ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
    find_library(log-lib log)
    target_link_libraries(xlua ${log-lib})
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "Switch")
    add_library(xlua STATIC ${LUA_CORE} ${LUA_LIB} ${XLUA_CORE} ${THIRDPART_SRC})
    target_compile_options(xlua PRIVATE -m64 -mcpu=cortex-a57+fp+simd+crypto+crc -fno-common -fno-short-enums -ffunction-sections -fdata-sections -fPIC -fms-extensions)
else()
    add_library(xlua SHARED ${LUA_CORE} ${LUA_LIB} ${LUA_SOCKET} ${XLUA_CORE} ${THIRDPART_SRC})
endif()

# Link settings for different platforms
if(WIN32 AND NOT CYGWIN)
    target_compile_definitions(xlua PRIVATE LUA_BUILD_AS_DLL)
    if(USING_LUAJIT)
        target_link_libraries(xlua ${CMAKE_SOURCE_DIR}/${LUAJIT_SRC_PATH}/lua51.lib ws2_32 ${THIRDPART_LIB})
    else()
        target_link_libraries(xlua ws2_32 ${THIRDPART_LIB})
    endif()
else()
    if(USING_LUAJIT AND NOT APPLE)
        target_link_libraries(xlua ${CMAKE_SOURCE_DIR}/${LUAJIT_SRC_PATH}/libluajit.a m ${THIRDPART_LIB})
    else()
        target_link_libraries(xlua m ${THIRDPART_LIB})
    endif()
endif()
