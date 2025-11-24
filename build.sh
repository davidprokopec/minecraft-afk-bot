#!/bin/bash
# Build script for mc-afker project

set -e

# Default values
BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_DIR="${BUILD_DIR:-build}"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}mc-afker Build Script${NC}"
echo "Build type: ${BUILD_TYPE}"
echo "Build directory: ${BUILD_DIR}"
echo ""

# Create build directory
echo -e "${YELLOW}Creating build directory...${NC}"
mkdir -p "${BUILD_DIR}"

# Configure CMake
echo -e "${YELLOW}Configuring CMake...${NC}"
cd "${BUILD_DIR}"
cmake .. -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"

# Build
echo -e "${YELLOW}Building project...${NC}"
cmake --build . --config "${BUILD_TYPE}"

echo -e "${GREEN}Build completed successfully!${NC}"
echo "Binary location: ${BUILD_DIR}/bin/mc-afker"

