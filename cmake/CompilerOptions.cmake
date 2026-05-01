# CompilerOptions.cmake
# Platform-specific compiler flags, optimisation settings, and sanitiser support.
#
# Usage:
#   include(cmake/CompilerOptions.cmake)
#   xaimassist_apply_compiler_options(<target>)
#
# CMake cache options exposed:
#   XAIMASSIST_ENABLE_SANITIZERS  — enable ASAN + UBSAN in Debug builds (GCC/Clang)
#   XAIMASSIST_NATIVE_ARCH        — add -march=native for local non-distributable builds

option(XAIMASSIST_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UBSan in Debug builds (GCC/Clang only)" OFF)

option(XAIMASSIST_NATIVE_ARCH
    "Optimise for the host CPU microarchitecture (-march=native). Do NOT enable for distributed binaries." OFF)

# ---------------------------------------------------------------------------
# Check for IPO/LTO support once at include time so all targets can query it.
# ---------------------------------------------------------------------------
include(CheckIPOSupported)
check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
if(NOT _ipo_supported)
    message(STATUS "IPO/LTO not supported by this toolchain: ${_ipo_output}")
endif()

# ---------------------------------------------------------------------------
# Main helper function
# ---------------------------------------------------------------------------
function(xaimassist_apply_compiler_options target)

    # -----------------------------------------------------------------------
    # Common settings (all platforms)
    # -----------------------------------------------------------------------
    set_target_properties(${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
    )

    # -----------------------------------------------------------------------
    # MSVC (Windows — cl.exe)
    # -----------------------------------------------------------------------
    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4             # Comprehensive warnings
            /permissive-    # Strict standard conformance
            /Zc:__cplusplus # Correct __cplusplus value
            /Zc:throwingNew # Assume new throws on failure (enables elision)
            /MP             # Parallel compilation across source files
            /utf-8          # Source and execution charset = UTF-8
            /wd4068         # Suppress unknown-pragma warnings (for GCC attrs)
        )

        # Release: maximise throughput
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Release>:/O2>       # Maximise speed
            $<$<CONFIG:Release>:/Ob2>      # Aggressive inlining
            $<$<CONFIG:Release>:/Oi>       # Intrinsic functions
            $<$<CONFIG:Release>:/Ot>       # Favour fast code
            $<$<CONFIG:Release>:/GS->      # Disable buffer security check (perf)
        )

        # Whole-program optimisation (LTO equivalent on MSVC)
        if(_ipo_supported)
            set_target_properties(${target} PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
            )
        endif()

        # Windows: silence deprecation warnings for POSIX names
        target_compile_definitions(${target} PRIVATE
            $<$<CONFIG:Release>:NDEBUG>
            _CRT_SECURE_NO_WARNINGS
            _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
        )

    # -----------------------------------------------------------------------
    # GCC / Clang (Linux, macOS, MinGW)
    # -----------------------------------------------------------------------
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wshadow
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wno-missing-field-initializers  # Qt-generated code triggers this
            -Wno-unused-parameter            # Interfaces leave params unnamed
        )

        # GCC-specific extras
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target} PRIVATE
                -Wlogical-op          # Suspicious use of logical operators
                -Wduplicated-cond     # Identical conditions in if/else-if
                -Wduplicated-branches # Identical branches in if/else
            )
        endif()

        # Clang-specific extras
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_compile_options(${target} PRIVATE
                -Wno-gnu-zero-variadic-macro-arguments  # Qt macros use this
            )
        endif()

        # Release optimisations
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Release>:-O3>
            $<$<CONFIG:Release>:-funroll-loops>
        )

        if(XAIMASSIST_NATIVE_ARCH)
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-march=native>
                $<$<CONFIG:Release>:-mtune=native>
            )
        endif()

        # LTO for Release (reduces binary size, improves inlining across TUs)
        if(_ipo_supported)
            set_target_properties(${target} PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
            )
        endif()

        # Debug build: sanitisers
        if(XAIMASSIST_ENABLE_SANITIZERS)
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
                $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
            )
            target_link_options(${target} PRIVATE
                $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            )
            message(STATUS "  [${target}] ASAN + UBSAN enabled for Debug builds")
        endif()

        # Linux: expose as many symbols as we want but hide everything else
        # to prevent accidental ABI leakage from third-party static libs.
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
            target_compile_options(${target} PRIVATE
                $<$<CONFIG:Release>:-fvisibility=hidden>
                $<$<CONFIG:Release>:-fvisibility-inlines-hidden>
            )
        endif()

    endif()

    # -----------------------------------------------------------------------
    # Platform-specific GPU / graphics driver linker hints (Linux)
    # On Linux with NVIDIA proprietary drivers, linking against libGL.so
    # directly rather than libOpenGL.so avoids the Mesa fallback path.
    # -----------------------------------------------------------------------
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Release>:-Wl,--as-needed>   # Strip unused shared libs
        )
    endif()

endfunction()
