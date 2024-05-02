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
//#include "helloworld_hero1.h"
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
#ifdef __HERO_1
    //uart_init(0x3002000, 50000000, 115200);
#endif
    tmp_1 = tmp_2;
  }

  printf("tmp_1 = %i\n", tmp_1);

  uint64_t phys_ptr;
  uint32_t *virt_ptr = hero_dev_l3_malloc(NULL, 64*sizeof(uint32_t), &phys_ptr);
  memset(virt_ptr, 0xf0, 64*sizeof(uint32_t));

  printf("%llx = %x\n", phys_ptr, *virt_ptr);

#pragma omp target device(1) map(to: phys_ptr, virt_ptr[0:64])
  {
    printf("%x %x\n\r", phys_ptr);
    printf("%x %x\n\r", virt_ptr[0]);
    printf("%x %x\n\r", *(uint32_t*)phys_ptr);
#ifdef __HERO_1
    //printf("%x %x\n\r", dma_1d_memcpy_from_host(0x81000000, 0x80000000, 0x100));
    //printf("%x %x\n\r", );
    //dma_1d_memcpy_from_host(0x81000000, phys_ptr, 0x1000000);
#endif
    uint32_t a = phys_ptr;
  }

/*
#pragma omp target device(1) map(to:tmp_1) map(from:tmp_2)
  {
#ifdef __HERO_1
    tmp_2 = tmp_1;
#else
    tmp_2 = tmp_1;
#endif
  }
*/
  return 0;
}
