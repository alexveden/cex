#!/bin/bash

# Script to check environment and compile source code from GitHub
# Exit on any error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
    exit 1
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# GitHub URL (you can change this as needed)
GITHUB_URL="https://raw.githubusercontent.com/alexveden/cex/refs/heads/master/cex.h"

# Step 1: Check if current directory is empty
print_status "Checking if current directory is empty..."
if [ "$(ls -A . 2>/dev/null)" ]; then
    print_error "Current directory is not empty. Please run this script in an empty directory for a new project."
fi

print_success "Directory is empty."

# Step 2: Check for curl or wget
print_status "Checking for download tools..."
if command -v curl &> /dev/null; then
    DOWNLOAD_TOOL="curl"
    DOWNLOAD_CMD="curl -L -o"
    print_success "Found curl"
elif command -v wget &> /dev/null; then
    DOWNLOAD_TOOL="wget"
    DOWNLOAD_CMD="wget -O"
    print_success "Found wget"
else
    print_error "Neither curl nor wget is available. Please install one of them."
fi

# Step 5: Check for compiler
print_status "Checking for compilers..."
if command -v clang &> /dev/null; then
    COMPILER="clang"
    print_success "Found clang compiler"
elif command -v gcc &> /dev/null; then
    COMPILER="gcc"
    print_success "Found gcc compiler"
else
    print_error "Neither clang nor gcc compiler is available. Please install one of them."
fi

# Step 3: Download source code
print_status "Downloading cex.h from $GITHUB_URL..."
$DOWNLOAD_CMD cex.h "$GITHUB_URL"

if [ ! -f "cex.h" ]; then
    print_error "Failed to download source code."
fi

print_success "Source code downloaded successfully."


# Compile the source code
print_status "Compiling with $COMPILER..."
$COMPILER -D CEX_NEW -x c ./cex.h -o cex

if [ $? -eq 0 ]; then
    print_success "Compilation successful!"
    print_success "Executable './cex' has been created."
else
    print_error "Compilation failed. Check the source code for errors."
fi

print_success "Executable './cex' has been created."
./cex
