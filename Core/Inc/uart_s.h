/*
 *
 */

#include <stdint.h>
#include "mutex.h"

#ifndef __INCLUDE_UART_S_H__
#define __INCLUDE_UART_S_H__

#define E_SUART_SUCCESS		(0)
#define E_SUART_PARM		(1)
#define E_SUART_BUSY		(2)

enum {
    S_UART_NONE = 0,
    S_UART_NOT_START,
    S_UART_START,
    S_UART_DATA,
    S_UART_PARITY,
    S_UART_STOP_1,
    S_UART_STOP_2,
    S_UART_EOT,
};

typedef struct tag_s_uart_t {
    mutex_t	mutex;
    uint32_t    bitspeed;
    uint8_t	ch_bits;
    char	par;
    uint8_t	stop;
    uint8_t	bits_transfered;
    uint32_t	tr_data;
    uint8_t	state;
    void	(*set_tx)(uint8_t level);
    void	(*on_send_done)(void);
    int		(*init)(struct tag_s_uart_t *, int, uint8_t, char, uint8_t stop);
    int		(*send)(struct tag_s_uart_t *, uint32_t data, void (*on_send_done)(void));
} s_uart_t;

extern s_uart_t	*s_uart;

#endif /* __INCLUDE_UART_S_H__ */
