/*
 *
 */

#include <stdint.h>
#include "mutex.h"
#include "rbuff_s.h"

int rbuff_s_init(void *mem, uint32_t size)
{
    rbuff_s *init = (rbuff_s *)mem;
    if (!mem) return E_RBS_PARAM;

    /* min 1 byte to fit and 1 byte guard interval */
    if (size < (sizeof(rbuff_s_ctrl) + 2))
	return E_RBS_PARAM;

    mutex_unlock(&init->ctrl.mutex);
    init->ctrl.head = 0;
    init->ctrl.tail = 0;
    init->ctrl.max_size = size - sizeof(rbuff_s_ctrl);

    return E_RBS_SUCESS;
}

int rbuff_s_put(rbuff_s *to, rbuff_s_data data)
{
    int err = mutex_try_lock(&to->ctrl.mutex);
    if (err != E_MTX_SUCCESS) return E_RBS_BUSY;

    /* put */
    to->data[to->ctrl.head] = data;
    /* move head */
    to->ctrl.head++;
    to->ctrl.head %= to->ctrl.max_size;
    /* check and move tail (if required) */
    if (to->ctrl.head == to->ctrl.tail) {
	to->ctrl.tail++;
        to->ctrl.tail %= to->ctrl.max_size;
    };

    mutex_unlock(&to->ctrl.mutex);

    return E_RBS_SUCESS;
}

int rbuff_s_get(rbuff_s *to, rbuff_s_data *data)
{
    int err = mutex_try_lock(&to->ctrl.mutex);
    if (err != E_MTX_SUCCESS) return E_RBS_BUSY;

    if (to->ctrl.head != to->ctrl.tail) {
	/* get */
	*data = to->data[to->ctrl.tail];
	/* move tail */
	to->ctrl.tail++;
        to->ctrl.tail %= to->ctrl.max_size;
    } else {
	err = E_RBS_EMPTY;
    }

    mutex_unlock(&to->ctrl.mutex);
    return err;
}

/* Non-public function, requre mutex locked by calller */
static uint32_t rbuff_s_get_used(rbuff_s *to)
{

    if (to->ctrl.head == to->ctrl.tail) return to->ctrl.max_size;
    if (to->ctrl.head > to->ctrl.tail)
	return (to->ctrl.head - to->ctrl.tail);
    else
	return (to->ctrl.max_size + to->ctrl.head - to->ctrl.tail);
}

int rbuff_s_used(rbuff_s *to, uint32_t *used)
{
    int err = mutex_try_lock(&to->ctrl.mutex);
    if (err != E_MTX_SUCCESS) return E_RBS_BUSY;

    *used = to->ctrl.max_size - rbuff_s_get_used(to);

    mutex_unlock(&to->ctrl.mutex);
    return E_RBS_SUCESS;
}
