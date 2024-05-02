#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

// EXECUTE ME IN ROOT

#define MIN(A, B) ((A) < (B) ? (A) : (B))

int main(int argc, char **argv) {
    int device_fd =
        open("/sys/bus/pci/devices/0000:01:00.0/resource0", O_RDWR | O_SYNC);
    void *all_mem = mmap(NULL, 0x01000000, PROT_READ | PROT_WRITE, MAP_SHARED,
                         device_fd, 0xc0000000);
    int ret = 0;

    printf("Mapped pcie device memory at  %p\n", all_mem);

    if (argc < 5) {
        printf("Error not enough args\n");
        return -1;
    }

    printf("argc=%i\n", argc);

    char *file_path = argv[1];
    size_t offset = strtol(argv[2], NULL, 16);
    size_t size = strtol(argv[3], NULL, 16);
    char mode = argv[4][0];

    printf("%s, %p, %p @ %p %c\n", file_path, offset, size, all_mem, mode);

    if (mode != 'r' && mode != 'w') {
        printf("Error file mode\n");
        return -1;
    }

    if (mode == 'r') {
        remove(file_path);
    }

    int data_file_fd = open(file_path, O_RDWR | O_CREAT | O_SYNC, 0666);

    //*((uint8_t*) all_mem + offset) = 0x1;

    uint64_t buf[8];
    uint8_t buf_2[8];
    size_t rest;

    if (mode == 'r') {
        for (size_t i = 0; i < size; i += sizeof(buf)) {
            if (size - i < sizeof(buf)) {
                for (int j = 0; j < sizeof(buf_2); j += sizeof(uint8_t)) {
                    buf_2[j] = ((uint8_t *)(all_mem + offset + i))[j];
                    printf("i:%llx j:%i %#02llx\n", i, j, buf_2[j]);
                }
                printf("\n");
                ret = write(data_file_fd, buf_2, sizeof(buf_2));
            } else {
                for (int j = 0; j < sizeof(buf); j += sizeof(uint64_t)) {
                    buf[j / 8] = ((uint64_t *)(all_mem + offset + i))[j / 8];
                    printf("i:%llx j/8:%i %#016llx\n", i, j / 8, buf[j / 8]);
                }
                printf("\n");
                ret = write(data_file_fd, buf, sizeof(buf));
            }
            if (!ret) {
                printf("Error file write\n");
                break;
            }
        }
    }
    if (mode == 'w') {
        ret = read(data_file_fd, buf, sizeof(buf));
        int i = 0;
        if (!ret) {
            printf("Error file write\n");
        }
        while (ret > 0) {
            if (ret < sizeof(buf)) {
                for (int j = 0; j < ret; j += sizeof(uint8_t)) {
                    ((uint8_t *)(all_mem + offset + i))[j] =
                        ((uint8_t *)buf)[j];
                    printf("%p (i:%llx j:%i) %#02llx -> %p\n", ((uint8_t *)(offset + i) + j), i, j,
                           ((uint8_t *)buf)[j],
                           ((uint8_t *)(all_mem + offset + i)));
                }
            } else {
                for (int j = 0; j < sizeof(buf); j += sizeof(uint64_t)) {
                    ((uint64_t *)(all_mem + offset + i))[j / 8] = buf[j / 8];
                    printf("%p (i:%llx j/8:%i %#016llx) -> %p\n", ((uint64_t *)(offset + i) + j), i, j / 8,
                           buf[j / 8], ((uint64_t *)(all_mem + offset + i) + j));
                }
            }
            ret = read(data_file_fd, buf, sizeof(buf));
            i += ret;
            sleep(0.01);
        }
    }

    // munmap(all_mem, 0xFFFFFFFF);
}