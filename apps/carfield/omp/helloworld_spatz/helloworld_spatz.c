////// HERO_1 includes /////
#ifdef __HERO_1

////// HOST includes /////
#else
#include <libhero/herodev.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
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

    hero_add_timestamp("end", __func__, 1);

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
