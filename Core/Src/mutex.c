/*
 *
 */

#include "stm32f4xx_hal.h"
#include "mutex.h"

int mutex_try_lock(mutex_t *present)
{
    mutex_t new;

    new = __LDREXW(present);
    if (new != MUTEX_UNLOCKED) {
	__CLREX();
	return E_MTX_ALREADY_LOCKED;
    };
    new = MUTEX_LOCKED;
    if (__STREXW(new, present))
	return E_MTX_TRY_AGAIN;
    else
	return E_MTX_SUCCESS;
}

void mutex_unlock(mutex_t *present)
{
    *present = MUTEX_UNLOCKED;
}
