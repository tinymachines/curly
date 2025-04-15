#ifndef CURLY_H
#define CURLY_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <jansson.h>

/**
 * Error codes for Curly library
 */
typedef enum {
    CURLY_OK = 0,
    CURLY_ERROR_INVALID_JSON,
    CURLY_ERROR_MISSING_URL,
    CURLY_ERROR_CURL_INIT,
    CURLY_ERROR_CURL_PERFORM,
    CURLY_ERROR_MEMORY_ALLOCATION,
    CURLY_ERROR_UNKNOWN
} curly_error_t;

/**
 * Structure to hold response data
 */
typedef struct {
    char *data;
    size_t size;
} curly_response_t;

/**
 * Structure for HTTP request configuration
 */
typedef struct {
    char *url;
    char *method;
    json_t *headers;
    json_t *data;
    json_t *form;
    json_t *auth;
    json_t *cookies;
    int follow_redirects;
    int max_redirects;
    int timeout;
    json_t *retry;
    int verbose;
} curly_config_t;

/**
 * Parse JSON configuration and initialize curly_config_t
 *
 * @param json_str JSON string containing configuration
 * @param config Pointer to config structure to be initialized
 * @return CURLY_OK on success, error code otherwise
 */
curly_error_t curly_parse_config(const char *json_str, curly_config_t *config);

/**
 * Execute a request based on the provided configuration
 *
 * @param config Request configuration
 * @param response Pointer to response structure to be filled
 * @return CURLY_OK on success, error code otherwise
 */
curly_error_t curly_perform_request(const curly_config_t *config, curly_response_t *response);

/**
 * Free resources allocated for config structure
 *
 * @param config Pointer to config structure to be freed
 */
void curly_free_config(curly_config_t *config);

/**
 * Free resources allocated for response structure
 *
 * @param response Pointer to response structure to be freed
 */
void curly_free_response(curly_response_t *response);

/**
 * Get a string description for a curly_error_t code
 *
 * @param error The error code
 * @return String description of the error
 */
const char *curly_strerror(curly_error_t error);

#endif /* CURLY_H */
