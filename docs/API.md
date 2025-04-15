# Curly API Documentation

## Library API

Curly provides a simple C API for creating HTTP requests from JSON configurations.

### Data Structures

#### curly_error_t

Enumeration of error codes returned by Curly functions.

```c
typedef enum {
    CURLY_OK = 0,                   // Success
    CURLY_ERROR_INVALID_JSON,       // Invalid JSON format
    CURLY_ERROR_MISSING_URL,        // Missing URL in configuration
    CURLY_ERROR_CURL_INIT,          // Failed to initialize libcurl
    CURLY_ERROR_CURL_PERFORM,       // Failed to perform curl request
    CURLY_ERROR_MEMORY_ALLOCATION,  // Memory allocation failed
    CURLY_ERROR_UNKNOWN             // Unknown error
} curly_error_t;
```

#### curly_config_t

Structure for HTTP request configuration.

```c
typedef struct {
    char *url;               // Target URL
    char *method;            // HTTP method (GET, POST, etc.)
    json_t *headers;         // HTTP headers
    json_t *data;            // Request body data
    json_t *form;            // Form data
    json_t *auth;            // Authentication info
    json_t *cookies;         // Cookie configuration
    int follow_redirects;    // Whether to follow redirects
    int max_redirects;       // Maximum number of redirects
    int timeout;             // Connection timeout in seconds
    json_t *retry;           // Retry configuration
    int verbose;             // Verbose output flag
} curly_config_t;
```

#### curly_response_t

Structure to hold HTTP response data.

```c
typedef struct {
    char *data;              // Response body
    size_t size;             // Size of response data
} curly_response_t;
```

### Functions

#### curly_parse_config

Parse JSON configuration and initialize curly_config_t.

```c
curly_error_t curly_parse_config(const char *json_str, curly_config_t *config);
```

**Parameters**:
- `json_str`: JSON string containing configuration
- `config`: Pointer to config structure to be initialized

**Returns**:
- `CURLY_OK` on success
- Error code otherwise

**Example**:
```c
const char *json = "{\"url\":\"https://example.com\"}";
curly_config_t config;
curly_error_t error = curly_parse_config(json, &config);
if (error != CURLY_OK) {
    fprintf(stderr, "Error: %s\n", curly_strerror(error));
    return 1;
}
```

#### curly_perform_request

Execute a request based on the provided configuration.

```c
curly_error_t curly_perform_request(const curly_config_t *config, curly_response_t *response);
```

**Parameters**:
- `config`: Request configuration
- `response`: Pointer to response structure to be filled

**Returns**:
- `CURLY_OK` on success
- Error code otherwise

**Example**:
```c
curly_response_t response;
error = curly_perform_request(&config, &response);
if (error != CURLY_OK) {
    fprintf(stderr, "Error: %s\n", curly_strerror(error));
    curly_free_config(&config);
    return 1;
}
printf("Response: %s\n", response.data);
```

#### curly_free_config

Free resources allocated for config structure.

```c
void curly_free_config(curly_config_t *config);
```

**Parameters**:
- `config`: Pointer to config structure to be freed

#### curly_free_response

Free resources allocated for response structure.

```c
void curly_free_response(curly_response_t *response);
```

**Parameters**:
- `response`: Pointer to response structure to be freed

#### curly_strerror

Get a string description for a curly_error_t code.

```c
const char *curly_strerror(curly_error_t error);
```

**Parameters**:
- `error`: The error code

**Returns**:
- String description of the error

## JSON Configuration Format

### Basic Request

```json
{
  "url": "https://api.example.com/users"
}
```

### POST Request with JSON Data

```json
{
  "url": "https://api.example.com/users",
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

### Basic Authentication

```json
{
  "url": "https://api.example.com/protected",
  "auth": {
    "type": "basic",
    "username": "user",
    "password": "pass"
  }
}
```

### Bearer Token Authentication

```json
{
  "url": "https://api.example.com/protected",
  "auth": {
    "type": "bearer",
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
  }
}
```

### Working with Cookies

```json
{
  "url": "https://website.com/login",
  "method": "POST",
  "data": {
    "username": "user",
    "password": "pass"
  },
  "cookies": {
    "save": "./cookies.txt"
  }
}
```

```json
{
  "url": "https://website.com/protected-area",
  "cookies": {
    "load": "./cookies.txt"
  }
}
```

### Advanced Options

```json
{
  "url": "https://example.com/resource",
  "method": "GET",
  "follow_redirects": true,
  "max_redirects": 5,
  "timeout": 30,
  "retry": {
    "count": 3,
    "delay": 2
  },
  "verbose": true
}
```

## Complete Example

```c
#include <stdio.h>
#include <stdlib.h>
#include "curly.h"

int main() {
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    
    const char *json_str = "{"
        "\"url\": \"https://httpbin.org/get\","
        "\"headers\": {"
            "\"User-Agent\": \"Curly/1.0\""
        "}"
    "}";
    
    curly_config_t config;
    curly_response_t response;
    
    // Parse JSON config
    curly_error_t error = curly_parse_config(json_str, &config);
    if (error != CURLY_OK) {
        fprintf(stderr, "Error: %s\n", curly_strerror(error));
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    // Perform the request
    error = curly_perform_request(&config, &response);
    if (error != CURLY_OK) {
        fprintf(stderr, "Error: %s\n", curly_strerror(error));
        curly_free_config(&config);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    // Print the response
    printf("%s\n", response.data);
    
    // Clean up
    curly_free_config(&config);
    curly_free_response(&response);
    curl_global_cleanup();
    
    return EXIT_SUCCESS;
}
```