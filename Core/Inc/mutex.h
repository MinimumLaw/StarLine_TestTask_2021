/*
 *
 */

#ifndef __INCLUDE_MUTEX_H__
#define __INCLUDE_MUTEX_H__

#include <stdint.h>

#define MUTEX_LOCKED		(1)
#define MUTEX_UNLOCKED		(0)

#define E_MTX_SUCCESS		(0)
#define E_MTX_ALREADY_LOCKED	(1)
#define E_MTX_TRY_AGAIN		(2)

typedef uint32_t mutex_t;

void mutex_unlock(mutex_t *);
int  mutex_try_lock(mutex_t *);

#endif /* __INCLUDE_MUTEX_H__ */
