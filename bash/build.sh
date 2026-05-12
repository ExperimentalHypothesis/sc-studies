#!/bin/bash

# Build script for ACSIL study with automatic deployment to Sierra Chart
#
# Usage:
#   ./build.sh                                     # Builds default study → default DLL
#   ./build.sh src/first_test/main.cpp             # Builds specified source → matching DLL
#   ./build.sh src/first_test/main.cpp MyStudyName # Builds specified source → MyStudyName.dll

set -euo pipefail

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Parse arguments
SOURCE_FILE="${1:-src/first_test/main.cpp}"
DLL_NAME="${2:-}"

# Resolve output base name (no extension).
# Default: parent folder name. Every study in this repo lives in
# src/<StudyName>/main.cpp, so `basename source.cpp .cpp` would always give
# `main` — useless. The folder name is what we actually want.
if [ -z "$DLL_NAME" ]; then
    DLL_BASE_NAME="$(basename "$(dirname "${SOURCE_FILE}")")"
else
    DLL_BASE_NAME="$DLL_NAME"
fi

echo -e "${BLUE}Building Sierra Chart study...${NC}"
echo -e "Source: ${YELLOW}${SOURCE_FILE}${NC}"
echo -e "Output: ${YELLOW}${DLL_BASE_NAME}.dll${NC}"

# Clean and create build directory
rm -rf build
mkdir -p build
cd build

# Configure with CMake using the MinGW cross-compiler toolchain
cmake_args=(
    -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-toolchain.cmake
    -DSOURCE_FILE="${SOURCE_FILE}"
)

if [ -n "$DLL_NAME" ]; then
    cmake_args+=(-DDLL_NAME="${DLL_NAME}")
fi

cmake "${cmake_args[@]}" ..

# Build the DLL
cmake --build .

DLL_PATH="$(pwd)/${DLL_BASE_NAME}.dll"

echo -e "${GREEN}Build complete!${NC}"
echo -e "DLL location: ${DLL_PATH}"
echo -e "Configure SIERRACHART_DIR in CMake to enable auto-copy after build."
