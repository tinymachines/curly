#include "curly.h"

// Structure for storing data received from libcurl
struct curl_write_data {
    char *data;
    size_t size;
};

// Custom strdup implementation if not available
static char *safe_strdup(const char *str) {
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char *new_str = malloc(len);
    if (new_str == NULL) {
        return NULL;
    }
    
    return memcpy(new_str, str, len);
}

// Callback function for libcurl to write received data
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t realsize = size * nmemb;
    struct curl_write_data *write_data = (struct curl_write_data *)userdata;

    char *new_data = realloc(write_data->data, write_data->size + realsize + 1);
    if (new_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for response data\n");
        return 0;  // Signal error to libcurl
    }

    write_data->data = new_data;
    memcpy(&(write_data->data[write_data->size]), ptr, realsize);
    write_data->size += realsize;
    write_data->data[write_data->size] = '\0';  // Null-terminate the string

    return realsize;
}

// Initialize config with default values
static void init_config(curly_config_t *config) {
    if (config) {
        memset(config, 0, sizeof(curly_config_t));
        config->method = safe_strdup("GET");  // Default method is GET
        config->timeout = 30;  // Default timeout is 30 seconds
        config->follow_redirects = 1;  // Follow redirects by default
        config->max_redirects = 10;  // Maximum 10 redirects by default
    }
}

curly_error_t curly_parse_config(const char *json_str, curly_config_t *config) {
    if (!json_str || !config) {
        return CURLY_ERROR_INVALID_JSON;
    }

    // Initialize config with default values
    init_config(config);

    json_error_t json_error;
    json_t *root = json_loads(json_str, 0, &json_error);
    if (!root) {
        fprintf(stderr, "JSON parse error: %s\n", json_error.text);
        return CURLY_ERROR_INVALID_JSON;
    }

    // Parse URL (required field)
    json_t *url = json_object_get(root, "url");
    if (!url || !json_is_string(url)) {
        json_decref(root);
        return CURLY_ERROR_MISSING_URL;
    }
    config->url = safe_strdup(json_string_value(url));

    // Parse method (optional, default is GET)
    json_t *method = json_object_get(root, "method");
    if (method && json_is_string(method)) {
        free(config->method); // Free default value
        config->method = safe_strdup(json_string_value(method));
    }

    // Parse headers (optional)
    json_t *headers = json_object_get(root, "headers");
    if (headers && json_is_object(headers)) {
        config->headers = json_deep_copy(headers);
    }

    // Parse data (optional)
    json_t *data = json_object_get(root, "data");
    if (data) {
        config->data = json_deep_copy(data);
    }

    // Parse form data (optional)
    json_t *form = json_object_get(root, "form");
    if (form && json_is_object(form)) {
        config->form = json_deep_copy(form);
    }

    // Parse auth (optional)
    json_t *auth = json_object_get(root, "auth");
    if (auth && json_is_object(auth)) {
        config->auth = json_deep_copy(auth);
    }

    // Parse cookies (optional)
    json_t *cookies = json_object_get(root, "cookies");
    if (cookies && json_is_object(cookies)) {
        config->cookies = json_deep_copy(cookies);
    }

    // Parse follow_redirects (optional)
    json_t *follow_redirects = json_object_get(root, "follow_redirects");
    if (follow_redirects && json_is_boolean(follow_redirects)) {
        config->follow_redirects = json_is_true(follow_redirects) ? 1 : 0;
    }

    // Parse max_redirects (optional)
    json_t *max_redirects = json_object_get(root, "max_redirects");
    if (max_redirects && json_is_integer(max_redirects)) {
        config->max_redirects = (int)json_integer_value(max_redirects);
    }

    // Parse timeout (optional)
    json_t *timeout = json_object_get(root, "timeout");
    if (timeout && json_is_integer(timeout)) {
        config->timeout = (int)json_integer_value(timeout);
    }

    // Parse retry (optional)
    json_t *retry = json_object_get(root, "retry");
    if (retry && json_is_object(retry)) {
        config->retry = json_deep_copy(retry);
    }

    // Parse verbose (optional)
    json_t *verbose = json_object_get(root, "verbose");
    if (verbose && json_is_boolean(verbose)) {
        config->verbose = json_is_true(verbose) ? 1 : 0;
    }

    json_decref(root);
    return CURLY_OK;
}

// Helper to set headers from JSON object
static CURLcode set_headers(CURL *curl, const json_t *headers) {
    struct curl_slist *header_list = NULL;
    const char *key;
    json_t *value;

    json_object_foreach((json_t *)headers, key, value) {
        if (json_is_string(value)) {
            char *header = NULL;
            int header_len = snprintf(NULL, 0, "%s: %s", key, json_string_value(value));
            if (header_len < 0) {
                curl_slist_free_all(header_list);
                return CURLE_OUT_OF_MEMORY;
            }
            
            header = malloc(header_len + 1);
            if (!header) {
                curl_slist_free_all(header_list);
                return CURLE_OUT_OF_MEMORY;
            }
            
            snprintf(header, header_len + 1, "%s: %s", key, json_string_value(value));
            header_list = curl_slist_append(header_list, header);
            free(header);
        }
    }

    if (header_list) {
        return curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    }
    
    return CURLE_OK;
}

// Helper to set auth options
static CURLcode set_auth(CURL *curl, const json_t *auth) {
    json_t *type = json_object_get(auth, "type");
    if (!type || !json_is_string(type)) {
        return CURLE_BAD_FUNCTION_ARGUMENT;
    }

    const char *auth_type = json_string_value(type);
    
    if (strcmp(auth_type, "basic") == 0) {
        json_t *username = json_object_get(auth, "username");
        json_t *password = json_object_get(auth, "password");
        
        if (username && json_is_string(username) && 
            password && json_is_string(password)) {
            char *userpass = NULL;
            int userpass_len = snprintf(NULL, 0, "%s:%s", 
                                      json_string_value(username), 
                                      json_string_value(password));
            
            if (userpass_len < 0) {
                return CURLE_OUT_OF_MEMORY;
            }
            
            userpass = malloc(userpass_len + 1);
            if (!userpass) {
                return CURLE_OUT_OF_MEMORY;
            }
            
            snprintf(userpass, userpass_len + 1, "%s:%s", 
                    json_string_value(username), 
                    json_string_value(password));
                    
            CURLcode res = curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
            if (res != CURLE_OK) {
                free(userpass);
                return res;
            }
            
            res = curl_easy_setopt(curl, CURLOPT_USERPWD, userpass);
            free(userpass);
            return res;
        }
    } else if (strcmp(auth_type, "bearer") == 0) {
        json_t *token = json_object_get(auth, "token");
        if (token && json_is_string(token)) {
            char *auth_header = NULL;
            int header_len = snprintf(NULL, 0, "Authorization: Bearer %s", 
                                     json_string_value(token));
            
            if (header_len < 0) {
                return CURLE_OUT_OF_MEMORY;
            }
            
            auth_header = malloc(header_len + 1);
            if (!auth_header) {
                return CURLE_OUT_OF_MEMORY;
            }
            
            snprintf(auth_header, header_len + 1, "Authorization: Bearer %s", 
                    json_string_value(token));
                    
            struct curl_slist *header_list = curl_slist_append(NULL, auth_header);
            free(auth_header);
            
            if (!header_list) {
                return CURLE_OUT_OF_MEMORY;
            }
            
            return curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
        }
    }
    
    return CURLE_BAD_FUNCTION_ARGUMENT;
}

// Helper to set cookies
static CURLcode set_cookies(CURL *curl, const json_t *cookies) {
    json_t *save = json_object_get(cookies, "save");
    if (save && json_is_string(save)) {
        return curl_easy_setopt(curl, CURLOPT_COOKIEJAR, json_string_value(save));
    }
    
    json_t *load = json_object_get(cookies, "load");
    if (load && json_is_string(load)) {
        return curl_easy_setopt(curl, CURLOPT_COOKIEFILE, json_string_value(load));
    }
    
    return CURLE_OK;
}

// Helper to set JSON data for POST/PUT
static CURLcode set_json_data(CURL *curl, const json_t *data) {
    char *json_str = json_dumps(data, JSON_COMPACT);
    if (!json_str) {
        return CURLE_OUT_OF_MEMORY;
    }
    
    CURLcode res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
    if (res != CURLE_OK) {
        free(json_str);
        return res;
    }
    
    // Store the json_str somewhere so it can be freed after the request completes
    // For now, this is a memory leak in this example
    
    return CURLE_OK;
}

curly_error_t curly_perform_request(const curly_config_t *config, curly_response_t *response) {
    if (!config || !response || !config->url) {
        return CURLY_ERROR_INVALID_JSON;
    }
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        return CURLY_ERROR_CURL_INIT;
    }
    
    // Initialize response
    response->data = malloc(1);
    if (!response->data) {
        curl_easy_cleanup(curl);
        return CURLY_ERROR_MEMORY_ALLOCATION;
    }
    response->data[0] = '\0';
    response->size = 0;
    
    struct curl_write_data write_data;
    write_data.data = malloc(1);
    if (!write_data.data) {
        free(response->data);
        curl_easy_cleanup(curl);
        return CURLY_ERROR_MEMORY_ALLOCATION;
    }
    write_data.data[0] = '\0';
    write_data.size = 0;
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, config->url);
    
    // Set write callback
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_data);
    
    // Set HTTP method
    if (strcmp(config->method, "GET") != 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, config->method);
    }
    
    // Set headers if provided
    if (config->headers) {
        set_headers(curl, config->headers);
    }
    
    // Set data if provided (for POST, PUT, etc.)
    if (config->data) {
        set_json_data(curl, config->data);
    }
    
    // Set auth if provided
    if (config->auth) {
        set_auth(curl, config->auth);
    }
    
    // Set cookies if provided
    if (config->cookies) {
        set_cookies(curl, config->cookies);
    }
    
    // Set follow_redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, config->follow_redirects);
    
    // Set max_redirects if follow_redirects is enabled
    if (config->follow_redirects) {
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, config->max_redirects);
    }
    
    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config->timeout);
    
    // Set verbose mode
    curl_easy_setopt(curl, CURLOPT_VERBOSE, config->verbose);
    
    // Perform the request
    CURLcode curl_res = curl_easy_perform(curl);
    
    if (curl_res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(curl_res));
        free(write_data.data);
        curl_easy_cleanup(curl);
        return CURLY_ERROR_CURL_PERFORM;
    }
    
    // Copy the response data
    response->data = write_data.data;
    response->size = write_data.size;
    
    curl_easy_cleanup(curl);
    return CURLY_OK;
}

void curly_free_config(curly_config_t *config) {
    if (!config) return;
    
    free(config->url);
    free(config->method);
    
    if (config->headers) json_decref(config->headers);
    if (config->data) json_decref(config->data);
    if (config->form) json_decref(config->form);
    if (config->auth) json_decref(config->auth);
    if (config->cookies) json_decref(config->cookies);
    if (config->retry) json_decref(config->retry);
    
    // Reset the structure to all zeros
    memset(config, 0, sizeof(curly_config_t));
}

void curly_free_response(curly_response_t *response) {
    if (!response) return;
    
    free(response->data);
    response->data = NULL;
    response->size = 0;
}

const char *curly_strerror(curly_error_t error) {
    switch (error) {
        case CURLY_OK:
            return "Success";
        case CURLY_ERROR_INVALID_JSON:
            return "Invalid JSON format";
        case CURLY_ERROR_MISSING_URL:
            return "Missing URL in configuration";
        case CURLY_ERROR_CURL_INIT:
            return "Failed to initialize libcurl";
        case CURLY_ERROR_CURL_PERFORM:
            return "Failed to perform curl request";
        case CURLY_ERROR_MEMORY_ALLOCATION:
            return "Memory allocation failed";
        case CURLY_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}
