#!/bin/bash
set -ex

# ANSI color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Function to print colored messages
print_status() {
    echo -e "${GREEN}[+] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[!] $1${NC}"
}

print_error() {
    echo -e "${RED}[-] $1${NC}"
}

# Function to check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script requires root privileges for installation."
        print_error "Please run with: sudo $0"
        exit 1
    fi
}

# Check for dependencies
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=0
    
    # Check for gcc/cc
    if ! command -v gcc &> /dev/null && ! command -v cc &> /dev/null; then
        print_warning "C compiler (gcc or cc) not found"
        missing_deps=1
    fi
    
    # Check for make
    if ! command -v make &> /dev/null; then
        print_warning "make not found"
        missing_deps=1
    fi
    
    # Check for libcurl
    pkg-config --libs libcurl &> /dev/null
    if [ $? -ne 0 ]; then
        print_warning "libcurl development files not found"
        missing_deps=1
    fi
    
    # Check for jansson
    pkg-config --libs jansson &> /dev/null
    if [ $? -ne 0 ]; then
        print_warning "jansson development files not found"
        missing_deps=1
    fi
    
    if [ $missing_deps -eq 1 ]; then
        print_error "Missing dependencies. Please install them manually:"
        print_error "  For Debian/Ubuntu: sudo apt-get install build-essential libcurl4-openssl-dev libjansson-dev"
        print_error "  For RHEL/CentOS/Fedora: sudo yum install gcc make libcurl-devel jansson-devel"
        print_error "  For Arch Linux: sudo pacman -S base-devel curl jansson"
        print_error "  For Alpine Linux: sudo apk add build-base curl-dev jansson-dev"
        print_error "  For macOS: brew install curl jansson"
        exit 1
    fi
    
    print_status "All dependencies are installed."
}

# Build the project
build_project() {
    print_status "Building curly..."
    
    # Remove and recreate build directories with proper permissions
    rm -rf build bin
    mkdir -p build bin
    chmod 755 build bin
    
    make
    
    if [ $? -ne 0 ]; then
        print_error "Build failed."
        exit 1
    fi
    
    print_status "Build completed successfully."
}

# Run tests
run_tests() {
    print_status "Running tests..."
    make test
    
    if [ $? -ne 0 ]; then
        print_error "Tests failed."
        exit 1
    fi
    
    print_status "All tests passed successfully."
}

# Install using make install
install_binary() {
    print_status "Installing curly system-wide..."
    check_root
    make install
    
    print_status "Installation completed. You can now run 'curly' and 'curly_parallel' from anywhere."
}

# Main script
main() {
    print_status "Starting curly setup..."
    
    # Check dependencies
    check_dependencies
    
    # Build project
    build_project
    
    # Run tests
    run_tests
    
    # If -i or --install flag is provided, install the binary
    if [ "$1" == "-i" ] || [ "$1" == "--install" ]; then
        install_binary
    else
        print_warning "Binary not installed to system. To install, run:"
        print_warning "sudo $0 --install"
    fi
    
    print_status "Setup complete!"
    print_status "You can run curly locally with: ./bin/curly"
    
    # Print example usage
    echo
    print_status "Example usage:"
    echo "  ./bin/curly -f examples/basic_get.json"
    echo "  ./bin/curly -s '{\"url\":\"https://httpbin.org/get\"}'"
    echo "  ./bin/curly_parallel -i examples/parallel_download.tsv -t 4"
}

# Execute main function with all arguments
main "$@"