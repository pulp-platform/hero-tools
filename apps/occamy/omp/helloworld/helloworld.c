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

////// HOST includes /////
#else
#include <libhero/herodev.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#define snrt_printf printf
#endif
///// ALL includes /////
#include <hero_64.h>
///// END includes /////

int main(int argc, char *argv[]) {

    uint32_t tmp_1;
    uint32_t tmp_2;

    // Benchmark omp init

    hero_add_timestamp("enter_omp_init", __func__, 1);

#pragma omp target device(1)
    { asm volatile("nop"); }

    // Benchmark offloads

    for (int i = 0; i < 10; i++) {

        // Benchmark simple offload

        hero_add_timestamp("enter_omp_nop", __func__, 1);

#pragma omp target device(1)
        { asm volatile("nop"); }

        tmp_1 = 5;
        tmp_2 = 10;

        // Benchmark offload with data copy to

        hero_add_timestamp("enter_omp_map_to", __func__, 1);

#pragma omp target device(1) map(to : tmp_1)
        { volatile uint32_t local_tmp_1 = tmp_1; }

        // Benchmark offload with data copy to_from

        hero_add_timestamp("enter_omp_map_to_from", __func__, 1);

#pragma omp target device(1) map(tofrom : tmp_1, tmp_2)
        { tmp_1 = tmp_2; }

        if (tmp_1 != tmp_2)
            printf("Error: map to_from did not work");
    }

#ifndef __HERO_1
    // Print all the recorded timestamps
    hero_print_timestamp();

    // Print the device cycles per region
    for (int i = 0; i < hero_num_device_cycles; i++)
        printf("%u - ", hero_device_cycles[i]);
    printf("\n");
#endif

    return 0;
}
