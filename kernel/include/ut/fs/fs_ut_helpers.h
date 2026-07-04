/**
 * @file fs_ut_helpers.h
 * @brief Shared fixtures and helpers for the filesystem test suites.
 */
#pragma once
#include "fs/vfs.h"

int fs_test_setup(void);
void fs_test_teardown(void);
int fs_suite_setup(void);
int fs_suite_teardown(void);

// Utility functions
int remount();
int fs_create_test_file(const char *path, const char *content);
int fs_verify_file_content(const char *path, const char *expected);
int fs_file_exists(const char *path);
