/**
 * @file binary.h
 * @brief Bit-mask helpers.
 */
#pragma once

#define FLAG_SET(buf, flag_mask) buf |= (flag_mask)
#define FLAG_UNSET(buf, flag_mask) buf &= ~(flag_mask)
