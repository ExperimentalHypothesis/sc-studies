cmake_minimum_required(VERSION 3.20)

set(SOURCE_FILE "src/first_test/main.cpp" CACHE STRING "Path to the ACSIL study source file")
set(DLL_NAME "" CACHE STRING "Base name for the output DLL (without extension)")
set(SIERRACHART_DIR "" CACHE PATH "Optional Sierra Chart Data folder for post-build copy")

if(IS_ABSOLUTE "${SOURCE_FILE}")
    set(STUDY_SOURCE "${SOURCE_FILE}")
else()
    set(STUDY_SOURCE "${CMAKE_SOURCE_DIR}/${SOURCE_FILE}")
endif()

if(NOT EXISTS "${STUDY_SOURCE}")
    message(FATAL_ERROR "Study source file not found: ${STUDY_SOURCE}")
endif()

if(DLL_NAME STREQUAL "")
    # Default the DLL name to the source's parent folder (every study lives at
    # src/<StudyName>/main.cpp, so the file basename "main" is never useful).
    get_filename_component(_study_dir "${STUDY_SOURCE}" DIRECTORY)
    get_filename_component(_dll_base "${_study_dir}" NAME)
else()
    set(_dll_base "${DLL_NAME}")
endif()

project(${_dll_base} LANGUAGES CXX)

message(STATUS "Building study source: ${STUDY_SOURCE}")
message(STATUS "DLL output name    : ${_dll_base}.dll")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SC_STUDY_SOURCES "${STUDY_SOURCE}")

add_library(StudyDLL SHARED ${SC_STUDY_SOURCES})
target_include_directories(StudyDLL PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(SIERRACHART_DIR)
    target_include_directories(StudyDLL PRIVATE ${SIERRACHART_DIR})
endif()

set_target_properties(StudyDLL PROPERTIES
    PREFIX ""
    SUFFIX ".dll"
    OUTPUT_NAME "${_dll_base}"
)

if(WIN32 OR MINGW)
    target_compile_definitions(StudyDLL PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX _WIN32 _WINDOWS)
    # NOTE: Homebrew's mingw-w64 toolchain is built against the Universal CRT,
    # so the resulting DLL imports api-ms-win-crt-*.dll. Those are present on
    # Windows 10+ that's fully updated and on Wine 9+ with UCRT initialized
    # (Whisky: install Visual C++ Redistributable via the bottle's
    # configuration). To switch to legacy MSVCRT you need a different
    # toolchain such as llvm-mingw — the Homebrew mingw libs (libstdc++,
    # libmingwex, libgcc) all call UCRT internally and cannot be retargeted
    # via link flags alone.
    # -s strips the COFF symbol table at link time. Without this, GCC's
    # IPA-SRA / IPA-cp optimization clones (e.g. lambdas inside an SCSFExport
    # function) leak into the binary as globally-named non-export symbols
    # like `_ZZ12scsf_IML_REVENKUliE_clEi.isra.0`. Sierra Chart scans those
    # COFF symbols and mistakes them for study entry points, producing a
    # garbage "Error loading: <dll>.<mangled-symbol>.isra.dll" message.
    target_link_options(StudyDLL PRIVATE -static -static-libgcc -static-libstdc++ -s)
    target_link_libraries(StudyDLL PRIVATE ws2_32 winmm)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(StudyDLL PRIVATE
        -O2
        -Wall
        -Wno-class-memaccess
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-stringop-truncation
        -Wno-strict-aliasing
    )
endif()

if(SIERRACHART_DIR)
    add_custom_command(TARGET StudyDLL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SIERRACHART_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:StudyDLL>
            "${SIERRACHART_DIR}/${_dll_base}.dll"
        COMMENT "Copying ${_dll_base}.dll to Sierra Chart folder: ${SIERRACHART_DIR}"
    )
endif()
