# Curly: JSON Wrapper for cURL

Curly is a simple yet powerful JSON-based wrapper around cURL that makes API interactions more readable, reusable, and maintainable.

## Overview

Curly translates JSON configurations into appropriate cURL commands, executes them, and returns the results in a structured format. It simplifies complex cURL operations and provides a more intuitive interface for API interactions.

## Features

- 📄 **JSON-based configuration** - Express API calls in clean, readable JSON
- 🔄 **Full HTTP method support** - GET, POST, PUT, DELETE, PATCH, etc.
- 🔐 **Authentication** - Basic auth, Bearer tokens, etc.
- 🍪 **Cookie handling** - Save and load cookies for session management
- 📤 **Custom headers** - Set any HTTP headers for your requests
- 📦 **Request body** - Send JSON data, form data, and more
- ⏱️ **Timeout control** - Configure request timeouts
- 🔄 **Redirect handling** - Control redirect behavior
- 🔍 **Verbose mode** - Detailed output for debugging
- 🚀 **Parallel downloads** - Process thousands of downloads concurrently

## Installation

### Prerequisites

- C compiler (gcc/clang)
- libcurl development files
- jansson development files
- make

### Building from Source

```bash
# Clone the repository
git clone https://github.com/tinymachines/curly.git
cd curly

# Build the project
./build.sh

# Install (optional)
sudo ./build.sh --install
```

## Quick Start

### Basic GET Request

```json
{
  "url": "https://httpbin.org/get"
}
```

Save this to a file named `request.json` and run:

```bash
curly -f request.json
```

Or pass it directly:

```bash
curly -s '{"url":"https://httpbin.org/get"}'
```

### POST Request with JSON Data

```json
{
  "url": "https://httpbin.org/post",
  "method": "POST",
  "headers": {
    "Content-Type": "application/json",
    "Accept": "application/json"
  },
  "data": {
    "name": "John Doe",
    "email": "john@example.com"
  }
}
```

### Using Authentication

```json
{
  "url": "https://httpbin.org/basic-auth/user/pass",
  "auth": {
    "type": "basic",
    "username": "user",
    "password": "pass"
  }
}
```

### Parallel Downloading

For downloading multiple files in parallel, use the `curly_parallel` tool. Create a TSV file with URLs and destination paths:

```
https://example.com/file1.jpg	./downloads/file1.jpg
https://example.com/file2.jpg	./downloads/file2.jpg
https://example.com/file3.jpg	./downloads/file3.jpg
```

Then run:

```bash
# Use 8 download threads, read from stdin
cat urls.tsv | curly_parallel -t 8

# Or specify an input file
curly_parallel -i urls.tsv -t 16
```

The tool will create necessary directories, download all files in parallel, and report progress.

## Documentation

For more detailed information, see:

- [API Documentation](docs/API.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Status and Roadmap](docs/STATUS.md)

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.