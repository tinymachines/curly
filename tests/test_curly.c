#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <curl/curl.h>
#include "curly.h"

void test_parse_config_basic() {
    printf("Running test_parse_config_basic...\n");
    
    const char *json = "{\"url\":\"https://example.com\"}";
    curly_config_t config;
    
    curly_error_t error = curly_parse_config(json, &config);
    assert(error == CURLY_OK);
    assert(config.url != NULL);
    assert(strcmp(config.url, "https://example.com") == 0);
    assert(config.method != NULL);
    assert(strcmp(config.method, "GET") == 0); // Default method
    
    curly_free_config(&config);
    printf("test_parse_config_basic: PASSED\n");
}

void test_parse_config_full() {
    printf("Running test_parse_config_full...\n");
    
    const char *json = "{\
        \"url\": \"https://example.com\",\
        \"method\": \"POST\",\
        \"headers\": {\
            \"Content-Type\": \"application/json\",\
            \"Accept\": \"application/json\"\
        },\
        \"data\": {\
            \"name\": \"John Doe\",\
            \"email\": \"john@example.com\"\
        },\
        \"follow_redirects\": true,\
        \"timeout\": 60,\
        \"verbose\": true\
    }";
    
    curly_config_t config;
    curly_error_t error = curly_parse_config(json, &config);
    
    assert(error == CURLY_OK);
    assert(config.url != NULL);
    assert(strcmp(config.url, "https://example.com") == 0);
    assert(config.method != NULL);
    assert(strcmp(config.method, "POST") == 0);
    assert(config.headers != NULL);
    assert(config.data != NULL);
    assert(config.follow_redirects == 1);
    assert(config.timeout == 60);
    assert(config.verbose == 1);
    
    curly_free_config(&config);
    printf("test_parse_config_full: PASSED\n");
}

void test_error_handling() {
    printf("Running test_error_handling...\n");
    
    // Test missing URL
    const char *json_no_url = "{\"method\":\"GET\"}";
    curly_config_t config;
    curly_error_t error = curly_parse_config(json_no_url, &config);
    
    assert(error == CURLY_ERROR_MISSING_URL);
    
    // Test invalid JSON
    const char *invalid_json = "{\"url\":https://example.com}";
    error = curly_parse_config(invalid_json, &config);
    
    assert(error == CURLY_ERROR_INVALID_JSON);
    
    printf("test_error_handling: PASSED\n");
}

int main(int argc, char *argv[]) {
    // If a specific test was specified
    if (argc > 1) {
        const char *test_name = argv[1];
        
        if (strcmp(test_name, "test_parse_config_basic") == 0) {
            test_parse_config_basic();
            return 0;
        } else if (strcmp(test_name, "test_parse_config_full") == 0) {
            test_parse_config_full();
            return 0;
        } else if (strcmp(test_name, "test_error_handling") == 0) {
            test_error_handling();
            return 0;
        } else {
            fprintf(stderr, "Unknown test: %s\n", test_name);
            return 1;
        }
    }
    
    // Run all tests
    curl_global_init(CURL_GLOBAL_ALL);
    
    test_parse_config_basic();
    test_parse_config_full();
    test_error_handling();
    
    curl_global_cleanup();
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}
