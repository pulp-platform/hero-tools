#pragma once

#include <stdint.h>
#include <stdio.h>

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

#define pr_error(fmt, ...)                                            \
  ({                                                                  \
    if (LOG_ERROR <= libhero_log_level)                               \
      printf("[ERROR libsnitch:%s()] " fmt, __func__, ##__VA_ARGS__); \
  })
#define pr_warn(fmt, ...)                                             \
  ({                                                                  \
    if (LOG_WARN <= libhero_log_level)                                \
      printf("[WARN  libsnitch:%s()] " fmt, __func__, ##__VA_ARGS__); \
  })
#define pr_info(fmt, ...)                                             \
  ({                                                                  \
    if (LOG_INFO <= libhero_log_level)                                \
      printf("[INFO  libsnitch:%s()] " fmt, __func__, ##__VA_ARGS__); \
  })
#define pr_debug(fmt, ...)                                            \
  ({                                                                  \
    if (LOG_DEBUG <= libhero_log_level)                               \
      printf("[DEBUG libsnitch:%s()] " fmt, __func__, ##__VA_ARGS__); \
  })
#define pr_trace(fmt, ...)                                            \
  ({                                                                  \
    if (LOG_TRACE <= libhero_log_level)                               \
      printf("[TRACE libsnitch:%s()] " fmt, __func__, ##__VA_ARGS__); \
  })
