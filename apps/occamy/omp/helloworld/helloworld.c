/*
 * HERO HelloWorld Example Application
 *
 * Copyright 2018 ETH Zurich, University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


////// HERO_1 includes /////
#ifdef __HERO_1
#include <stdio.h>
////// HOST includes /////
#else
#include <stdint.h>
#include <stdio.h>
#include <omp.h>
#include <libhero/herodev.h>
#endif
///// ALL includes /////
#include <hero_64.h>
///// END includes /////

int main(int argc, char *argv[]) {

  printf("cva6 main()\n");

  uint32_t tmp_1 = 5;
  uint32_t tmp_2 = 10;

#pragma omp target device(1) map(tofrom: tmp_1, tmp_2)
  {
    tmp_1 = tmp_2;
  }

  printf("Got %u", tmp_1);

  return 0;
}
