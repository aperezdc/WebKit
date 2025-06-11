# Copyright (C) 2020 Igalia S.L.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1.  Redistributions of source code must retain the above copyright
#     notice, this list of conditions and the following disclaimer.
# 2.  Redistributions in binary form must reproduce the above copyright
#     notice, this list of conditions and the following disclaimer in the
#     documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND ITS CONTRIBUTORS ``AS
# IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ITS
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#[=======================================================================[.rst:
FindAndroid
-----------

Find Android NDK headers and libraries.

This module supports checking for the following components:

``Android``
  The base `libandroid` library. If no components are specified, this
  will the only one to search for.

Imported Targets
^^^^^^^^^^^^^^^^

For each supported component, the corresponding `Android::<name>` target
will be defined, for example `Android::Android`, if found.

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables in your project, for each
component given its `<name>`:

``Android_<name>_FOUND``
  True if the component `<name>` is available.
``Android_<name>_LIBRARY``
  The library to link against to use the component `<name>`.
``Android_<name>_INCLUDE_DIR``
  Where to find the headers for the component `<name>`.

#]=======================================================================]

if (NOT ANDROID)
    return()
endif ()

include(CMakePushCheckState)
include(CheckFunctionExists)

if (Android_FIND_REQUIRED)
    set(_AndroidMessageLevel FATAL_ERROR)
else ()
    set(_AndroidMessageLevel WARNING)
endif ()

function(_AndroidHandleComponent target)
    if (NOT (${target} IN_LIST Android_FIND_COMPONENTS))
        message(DEBUG "${target} not in ${Android_FIND_COMPONENTS}")
        return()
    endif ()

    if (TARGET Android::${target})
        return()
    endif ()
    cmake_parse_arguments(PARSE_ARGV 1 opt
        ""
        ""
        "HEADER;NAMES;SYMBOLS"
    )
    if (NOT opt_NAMES)
        message(FATAL_ERROR "Option NAMES not specified")
    endif ()

    find_library(Android_${target}_LIBRARY NAMES ${opt_NAMES})
    if (NOT Android_${target}_LIBRARY)
        set(Android_${target}_FOUND FALSE PARENT_SCOPE)
        return()
    endif ()

    if (opt_HEADER)
        find_path(Android_${target}_INCLUDE_DIR NAMES ${opt_HEADER})
        if (NOT Android_${target}_INCLUDE_DIR)
            message(FATAL_ERROR "Header for library Android::${target} not found, tried ${opt_HEADER}")
        endif ()
    endif ()

    if (opt_SYMBOLS)
        set(missing_symbols "")
        cmake_push_check_state(RESET)
        set(CMAKE_REQUIRED_LIBRARIES "${Android_${target}_LIBRARY}")

        foreach (symbol IN LISTS opt_SYMBOLS)
            check_function_exists("${symbol}" function_ok)
            if (NOT function_ok)
                list(APPEND missing_symbols "${symbol}")
            endif ()
        endforeach ()
        cmake_pop_check_state()

        if (missing_symbols)
            message(FATAL_ERROR
                "Library Android::${target} (${Android_${target}_LIBRARY}) "
                "is missing required functions ${missing_symbols}")
        endif ()
    endif ()

    add_library(Android::${target} UNKNOWN IMPORTED GLOBAL)
    set_target_properties(Android::${target} PROPERTIES
        IMPORTED_LOCATION "${Android_${target}_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${Android_${target}_INCLUDE_DIR}")
    set(Android_${target}_FOUND TRUE PARENT_SCOPE)
endfunction()

if (NOT Android_FIND_COMPONENTS)
    set(Android_FIND_COMPONENTS Android)
    set(Android_FIND_REQUIRED_Android ${Android_FIND_REQUIRED})
endif ()

_AndroidHandleComponent(Android
    NAMES android
    HEADER android/hardware_buffer.h
    SYMBOLS AHardwareBuffer_allocate
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DEFAULT_MSG HANDLE_COMPONENTS)
