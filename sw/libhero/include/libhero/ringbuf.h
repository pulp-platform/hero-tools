#pragma once

#include <stdint.h>
#include <string.h>

/**
 * @brief Ring buffer for simple communication from accelerator to host.
 * @tail: Points to the element in `data` which is read next
 * @head: Points to the element in `data` which is written next
 * @size: Number of elements in `data`. Head and tail pointer wrap at `size`
 * @element_size: Size of each element in bytes
 * @data_p: points to the base of the data buffer in physical address
 * @data_v: points to the base of the data buffer in virtual address space
 */
// Make sure this is 128-bits aligned for CVA6 d-cache
struct ring_buf {
  uint32_t head;
  uint32_t size;
  uint32_t tail;
  uint32_t element_size;
  uint64_t data_v;
  uint64_t data_p;
};

/**
 * @brief Copy data from `el` in the next free slot in the ring-buffer on the physical addresses
 *
 * @param rb pointer to the ring buffer struct
 * @param el pointer to the data to be copied into the ring buffer
 * @return int 0 on succes, -1 if the buffer is full
 */
static inline int rb_device_put(volatile struct ring_buf *rb, void *el) {
  uint32_t next_head = (rb->head + 1) % rb->size;
  // caught the tail, can't put data
  if (next_head == rb->tail)
    return -1;
  for (uint32_t i = 0; i < rb->element_size; i++)
    *((uint8_t *)rb->data_p + rb->element_size * rb->head + i) = *((uint8_t *)el + i);
  rb->head = next_head;
  return 0;
}
/**
 * @brief Pop element from ring buffer on virtual addresses
 *
 * @param rb pointer to ring buffer struct
 * @param el pointer to where element is copied to
 * @return 0 on success, -1 if no element could be popped
 */
static inline int rb_host_get(volatile struct ring_buf *rb, void *el) {
  // caught the head, can't get data
  if (rb->tail == rb->head)
    return -1;
  for (uint32_t i = 0; i < rb->element_size; i++)
    *((uint8_t *)el + i) = *((uint8_t *)rb->data_v + rb->element_size * rb->tail + i);
  rb->tail = (rb->tail + 1) % rb->size;
  return 0;
}

/**
 * @brief Copy data from `el` in the next free slot in the ring-buffer on the virtual addresses
 *
 * @param rb pointer to the ring buffer struct
 * @param el pointer to the data to be copied into the ring buffer
 * @return int 0 on success, -1 if the buffer is full
 */
static inline int rb_host_put(volatile struct ring_buf *rb, void *el) {
  uint32_t next_head = (rb->head + 1) % rb->size;
  // caught the tail, can't put data
  if (next_head == rb->tail)
    return -1;
  for (uint32_t i = 0; i < rb->element_size; i++) {
    *((uint8_t *)rb->data_v + rb->element_size * rb->head + i) = *((uint8_t *)el + i);
  }
  rb->head = next_head;
  return 0;
}
/**
 * @brief Pop element from ring buffer on physicl addresses
 *
 * @param rb pointer to ring buffer struct
 * @param el pointer to where element is copied to
 * @return 0 on success, -1 if no element could be popped
 */
static inline int rb_device_get(volatile struct ring_buf *rb, void *el) {
  // caught the head, can't get data
  if (rb->tail == rb->head)
    return -1;
  for (uint32_t i = 0; i < rb->element_size; i++)
    *((uint8_t *)el + i) = *((uint8_t *)rb->data_p + rb->element_size * rb->tail + i);
  rb->tail = (rb->tail + 1) % rb->size;
  return 0;
}
/**
 * @brief Init the ring buffer. See `struct ring_buf` for details
 */
static inline void rb_init(volatile struct ring_buf *rb, uint32_t size, uint32_t element_size) {
  rb->tail = 0;
  rb->head = 0;
  rb->size = size;
  rb->element_size = element_size;
}

static inline void dump_mbox(struct ring_buf *rbuf) {
  printf("---DUMPING NOW---\n\r");
  printf("mbox (%p)\n\r", rbuf);
  uint8_t* addr = (uint8_t*) rbuf;
  for(int i = 0; i < sizeof(struct ring_buf); i++) {
    if(i % 8 == 0)
        printf("\n\r(%p) : ", addr);
    printf("%#hhx-", *(addr++));
  }
  printf("\n\r");
  printf("head : %p = %u\n\r"     , &rbuf->head         , rbuf->head         );
  printf("size : %p = %u\n\r"     , &rbuf->size         , rbuf->size         );
  printf("tail : %p = %u\n\r"     , &rbuf->tail         , rbuf->tail         );
  printf("data_p : %p = %lx\n\r", &rbuf->data_p , rbuf->data_p       );
  printf("data_v : %p = %lx\n\r", &rbuf->data_v , rbuf->data_v       );
  //printf("tail %u, data_v %" PRIu64 ", element_size %u, size %u, data_p %" PRIu64 ", head %u\n\r", rbuf->tail, rbuf->data_v, rbuf->element_size, rbuf->size, rbuf->data_p, rbuf->head);
  printf("---DUMPING ENDS---\n\r");
}
