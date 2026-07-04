/**
 * @file sys_ioctl.h
 * @brief ioctl syscall.
 */
#pragma once
#include "klib/stdint.h"

/**
 * @brief Performs a device-specific control operation on an open fd.
 * @param fd Open file descriptor referring to a device.
 * @param request Device-specific request code.
 * @param arg Optional request argument (in or out, request-dependent).
 * @return 0 or a request-specific value on success, negative errno on failure.
 */
int sys_ioctl(int fd, unsigned long request, void *arg);
