#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stddef.h>

/* Test statistics */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;
static int assertions_total = 0;

/* Colors */
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET  "\033[0m"

/* Assert macros */
#define ASSERT_EQ(actual, expected) do { \
    assertions_total++; \
    if ((actual) != (expected)) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_EQ(%s, %s) => %lld != %lld\n" COLOR_RESET, \
               __FILE__, __LINE__, #actual, #expected, (long long)(actual), (long long)(expected)); \
        return 1; \
    } \
} while(0)

#define ASSERT_NEQ(actual, expected) do { \
    assertions_total++; \
    if ((actual) == (expected)) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_NEQ(%s, %s) => both %lld\n" COLOR_RESET, \
               __FILE__, __LINE__, #actual, #expected, (long long)(actual)); \
        return 1; \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    assertions_total++; \
    if (!(cond)) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_TRUE(%s)\n" COLOR_RESET, \
               __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while(0)

#define ASSERT_FALSE(cond) do { \
    assertions_total++; \
    if (cond) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_FALSE(%s)\n" COLOR_RESET, \
               __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    assertions_total++; \
    if ((ptr) != NULL) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_NULL(%s) => %p\n" COLOR_RESET, \
               __FILE__, __LINE__, #ptr, (void*)(ptr)); \
        return 1; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    assertions_total++; \
    if ((ptr) == NULL) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_NOT_NULL(%s)\n" COLOR_RESET, \
               __FILE__, __LINE__, #ptr); \
        return 1; \
    } \
} while(0)

#define ASSERT_STR_EQ(actual, expected) do { \
    assertions_total++; \
    if (strcmp((actual), (expected)) != 0) { \
        printf(COLOR_RED "  FAIL: %s:%d: ASSERT_STR_EQ\n    actual:   \"%s\"\n    expected: \"%s\"\n" COLOR_RESET, \
               __FILE__, __LINE__, (actual), (expected)); \
        return 1; \
    } \
} while(0)

/* Test runner */
typedef int (*test_func_t)(void);

typedef struct {
    const char* name;
    test_func_t func;
    const char* suite;
} test_entry_t;

#define TEST(name) static int test_##name(void)
#define TEST_ENTRY(name, suite) { #name, test_##name, suite }

#define RUN_TESTS(tests_array) do { \
    int total = sizeof(tests_array) / sizeof(tests_array[0]); \
    printf("\n" COLOR_YELLOW "═══════════════════════════════════════════════════\n"); \
    printf("  WebOS Test Suite v0.0.b\n"); \
    printf("  Running %d tests...\n", total); \
    printf("═══════════════════════════════════════════════════\n\n" COLOR_RESET); \
    \
    const char* current_suite = ""; \
    for (int i = 0; i < total; i++) { \
        if (strcmp(tests_array[i].suite, current_suite) != 0) { \
            current_suite = tests_array[i].suite; \
            printf(COLOR_YELLOW "\n[%s]\n" COLOR_RESET, current_suite); \
        } \
        tests_run++; \
        int result = tests_array[i].func(); \
        if (result == 0) { \
            tests_passed++; \
            printf(COLOR_GREEN "  ✓ %s\n" COLOR_RESET, tests_array[i].name); \
        } else { \
            tests_failed++; \
            printf(COLOR_RED "  ✗ %s\n" COLOR_RESET, tests_array[i].name); \
        } \
    } \
    \
    printf("\n" COLOR_YELLOW "═══════════════════════════════════════════════════\n"); \
    printf("  Results: %d/%d passed, %d failed, %d assertions\n", \
           tests_passed, tests_run, tests_failed, assertions_total); \
    if (tests_failed == 0) { \
        printf(COLOR_GREEN "  ALL TESTS PASSED ✓\n" COLOR_RESET); \
    } else { \
        printf(COLOR_RED "  %d TEST(S) FAILED ✗\n" COLOR_RESET, tests_failed); \
    } \
    printf(COLOR_YELLOW "═══════════════════════════════════════════════════\n\n" COLOR_RESET); \
    return tests_failed > 0 ? 1 : 0; \
} while(0)

#endif /* TEST_FRAMEWORK_H */
