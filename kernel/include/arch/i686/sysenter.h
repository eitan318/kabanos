/**
 * @file sysenter.h
 * @brief SYSENTER fast-syscall entry setup.
 */
#pragma once

/** @brief Programs the SYSENTER MSRs (CS, ESP, EIP) for syscall entry. */
void i686_sysenter_init();
