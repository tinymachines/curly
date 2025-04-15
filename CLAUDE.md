# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project: Curly - JSON Wrapper for cURL

### Build/Test Commands
- Build: `make` or `make all`
- Clean: `make clean`
- Test: `make test`
- Run single test: `make test TEST=test_name`
- Memory check: `make memcheck`
- Lint: `cppcheck --enable=all src/`

### Code Style Guidelines

#### C Specific Guidelines
- Use C99 standard
- 4 spaces for indentation, no tabs
- Use snake_case for functions and variables
- Use UPPER_CASE for constants and macros
- 80-100 character line length
- Function names should be verb_noun format

#### Memory Management
- Always check malloc/calloc return values
- Free all allocated memory, prevent leaks
- Use valgrind to verify memory handling
- Prefer stack allocation when appropriate

#### JSON Handling
- Use cJSON or jansson library for JSON parsing
- Validate all JSON input against schema
- Handle nested objects with careful null checking
- Use proper error codes and messages for malformed JSON

#### Error Handling
- Return error codes, not just error messages
- Log errors with appropriate detail level
- Check all libcurl return values
- Use consistent error reporting patterns

#### Security Considerations
- Sanitize all command line inputs
- Escape shell metacharacters
- Use libcurl's CURLOPT_WRITEFUNCTION for proper data handling
- Validate and sanitize URLs before use
- Handle credentials securely, never log sensitive data