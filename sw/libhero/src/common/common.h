#pragma once

// The device driver file
int device_fd;

struct card_alloc_arg {
    size_t size;
    uint64_t result_phys_addr;
    uint64_t result_virt_addr;
};

int driver_mmap(int device_fd, unsigned int page_offset, size_t length,
                  void **res) {
    *res = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, device_fd,
                page_offset * getpagesize());

    if (*res == MAP_FAILED) {
        printf("mmap() failed %s for offset: %x length: %lx\n", strerror(errno),
               page_offset, length);
        *res = NULL;
        return -EIO;
    }
    return 0;
}
