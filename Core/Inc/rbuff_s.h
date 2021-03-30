/*
 *
 */

#include <stdint.h>
#include "mutex.h"

#ifndef __INCLUDE_RINGBUFFER_SIMPLE_H__
#define __INCLUDE_RINGBUFFER_SIMPLE_H__

#define E_RBS_SUCESS		(0)
#define E_RBS_EMPTY		(1)
#define E_RBS_PARAM		(2)
#define E_RBS_BUSY		(3)

typedef uint8_t         rbuff_s_data;

typedef struct {
    mutex_t	mutex;
    uint32_t	head;
    uint32_t	tail;
    uint32_t	max_size;
} rbuff_s_ctrl;

typedef struct {
    rbuff_s_ctrl	ctrl;
    rbuff_s_data	data[];
} rbuff_s;

int rbuff_s_init(void *mem, uint32_t size);
int rbuff_s_put(rbuff_s *to, rbuff_s_data data);
int rbuff_s_get(rbuff_s *to, rbuff_s_data *data);
int rbuff_s_used(rbuff_s *to, uint32_t *used);

#endif /* __INCLUDE_RINGBUFFER_SIMPLE_H__ */
