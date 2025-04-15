# Curly Architecture

This document describes the architecture and design principles of the Curly project.

## Overview

Curly is a JSON-based wrapper around cURL that provides a simpler, more readable interface for making HTTP requests. It translates JSON configurations into appropriate cURL commands, executes them, and returns the results.

## Components

The architecture consists of the following core components:

### 1. JSON Parser

**Purpose**: Parse and validate JSON configuration.

**Implementation**:
- Uses the Jansson library for JSON parsing
- Validates required fields (e.g., URL)
- Handles default values for optional fields
- Provides clear error reporting for invalid JSON

### 2. Command Builder

**Purpose**: Translate JSON configuration into cURL options.

**Implementation**:
- Maps JSON properties to appropriate cURL options
- Sets up HTTP headers, method, data, authentication, etc.
- Handles different authentication types (Basic, Bearer)
- Manages cookies, redirects, and timeouts

### 3. Executor

**Purpose**: Execute the HTTP request using libcurl.

**Implementation**:
- Sets up curl handle with all necessary options
- Manages memory for request/response data
- Executes the HTTP request
- Handles response data via callbacks

### 4. Response Handler

**Purpose**: Process and return HTTP response data.

**Implementation**:
- Collects response data from callback
- Provides structured access to response content
- Handles memory cleanup

## Data Flow

1. JSON configuration is parsed into a `curly_config_t` structure
2. The configuration is validated for required fields
3. The configuration is used to set up a curl handle with appropriate options
4. The request is executed using libcurl
5. Response data is collected via callback
6. Response is returned to the caller
7. Resources are freed

## Memory Management

- All dynamic memory is tracked and freed appropriately
- Error paths clean up allocated resources
- Response data is allocated only once to avoid copies
- The API provides functions to free allocated resources

## Error Handling

- Enumerated error codes for different failure scenarios
- Clear error messages via `curly_strerror()`
- Validation of input parameters before use
- Proper checking of all libcurl return values

## Command-line Interface

- Simple interface for JSON file or string input
- Help documentation
- Error reporting
- Clean exit paths

## Testing Strategy

- Unit tests for core functionality
- Memory leak checking via valgrind
- Simple API design for easy testing
- Test coverage for both success and error paths

## Future Extensibility

The architecture is designed for future extensions:

- Additional JSON properties can be easily added
- New authentication methods can be supported
- Response handling can be extended for different formats
- The API is designed to be backward compatible
- Callback mechanisms allow for custom behavior