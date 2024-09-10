# Copyright 2018 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# This file is part of the HERCULES Compiler Passes for PREM transformation
# of code.

add_library(OmpHostPointerLegalizer
  SHARED
  OmpHostPointerLegalizer.cpp
)

install(TARGETS OmpHostPointerLegalizer LIBRARY DESTINATION lib/llvm-support)
