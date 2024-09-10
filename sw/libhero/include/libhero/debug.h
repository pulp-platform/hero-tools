// Copyright 2024 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
//
// Cyril Koenig <cykoenig@iis.ee.ethz.ch>

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

////////////////////
///// TIMING  //////
////////////////////

void hero_add_timestamp(char str_info[32], char str_func[32], int add_to_trace);
void hero_print_timestamp();

// Statically allocate timestamps for timing
#define MAX_TIMESTAMPS 1024
// 0 = CSR cycle ; 1 = linux cputime
#define HERO_TIMESTAMP_MODE 0
#define NS_PER_SECOND 1000000000

struct hero_timestamp {
  struct timespec timespec;
  uint64_t cycle;
  char str_info[32];
  char str_func[32];
  struct hero_timestamp* next;
};

// Host timestamps
extern int hero_num_timestamps;
extern struct hero_timestamp hero_timestamps[MAX_TIMESTAMPS];
// Device cycles (returned by device runtime)
extern int hero_num_device_cycles;
extern uint32_t hero_device_cycles[MAX_TIMESTAMPS];
extern int hero_num_dma_cycles;
extern uint32_t hero_dma_cycles[MAX_TIMESTAMPS];

/////////////////////
///// LOGGING  //////
/////////////////////

extern int libhero_log_level;

enum log_level {
  LOG_ERROR = 0,
  LOG_WARN = 1,
  LOG_INFO = 2,
  LOG_DEBUG = 3,
  LOG_TRACE = 4,
  LOG_MIN = LOG_ERROR,
  LOG_MAX = LOG_TRACE,
};

#define SHELL_RED "\033[0;31m"
#define SHELL_GRN "\033[0;32m"
#define SHELL_RST "\033[0m"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define pr_error(fmt, ...)                                            \
  ({                                                                  \
    if (LOG_ERROR <= libhero_log_level)                               \
      printf("[ERROR %s:%s()] " fmt, __FILENAME__, __func__, ##__VA_ARGS__); \
  })
#define pr_warn(fmt, ...)                                             \
  ({                                                                  \
    if (LOG_WARN <= libhero_log_level)                                \
      printf("[WARN  %s:%s()] " fmt, __FILENAME__, __func__, ##__VA_ARGS__); \
  })
#define pr_info(fmt, ...)                                             \
  ({                                                                  \
    if (LOG_INFO <= libhero_log_level)                                \
      printf("[INFO  %s:%s()] " fmt, __FILENAME__, __func__, ##__VA_ARGS__); \
  })
#define pr_debug(fmt, ...)                                            \
  ({                                                                  \
    if (LOG_DEBUG <= libhero_log_level)                               \
      printf("[DEBUG %s:%s()] " fmt, __FILENAME__, __func__, ##__VA_ARGS__); \
  })
#define pr_trace(fmt, ...)                                            \
  ({                                                                  \
    if (LOG_TRACE <= libhero_log_level)                               \
      printf("[TRACE %s:%s()] " fmt, __FILENAME__, __func__, ##__VA_ARGS__); \
  })

// If a call yields a nonzero return, return that immediately as an int.
#define CHECK_CALL(call, errmsg) \
    { \
        int __ccret = (volatile int)(call); \
        if (__ccret) { \
            pr_error(errmsg); \
            return __ccret; \
        } \
    }

// If a condition; if it is untrue, ummediately return an error code.
#define CHECK_ASSERT(ret, cond, errmsg) \
    if (!(cond)) {pr_error(errmsg); return (ret);}
