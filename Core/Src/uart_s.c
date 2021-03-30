/*
 *
 */

#include "stm32f4xx_hal.h"
#include "main.h"
#include "tim.h"
#include "uart_s.h"

static s_uart_t		uart_dev;
s_uart_t	*s_uart = &uart_dev;

/* ToDo: this is a test task, in real world s_uart list here */
static void on_bit_time(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM14) return; /* why we here? */

    switch(s_uart->state) {
    case S_UART_NOT_START:	/* Start transfer */
	s_uart->set_tx(0);
	s_uart->state = S_UART_DATA;
	s_uart->bits_transfered = 0;
	break;
    case S_UART_DATA:		/* start bit transfered */
        /* LSB first */
	s_uart->set_tx(s_uart->tr_data & (1<<s_uart->bits_transfered++));
	if (s_uart->bits_transfered >= s_uart->ch_bits) { /* FixMe: == here */
	    switch(s_uart->par) {	/* check no parity */
	    case 'n':
	    case 'N':
	    case ' ':
		s_uart->state = S_UART_STOP_1;
		break;
	    default:
		s_uart->state = S_UART_PARITY;
	    };
	} else { /* bits remaining - stay in data transfer state */
	    s_uart->state = S_UART_DATA;
	};
	break;
    case S_UART_PARITY:
	switch(s_uart->par) {
	case 'M': /* mark */
	case 'm':
	case '+':
	    s_uart->set_tx(1);
	    s_uart->state = S_UART_STOP_1;
	    break;
	case 'S': /* space */
	case 's':
	case '-':
	    s_uart->set_tx(0);
	    s_uart->state = S_UART_STOP_1;
	    break;
	case 'O': /* odd */
	case 'o': { /* Calc set bit numbers with  Brian Kernighan’s Algorithm */
            uint32_t tmp = s_uart->tr_data;
            uint8_t  nr = 0;

            while(tmp) {
              tmp &= (tmp - 1);
              nr++;
            };
	    s_uart->set_tx(!(nr & 1));
	    s_uart->state = S_UART_STOP_1; };
	    break;
	case 'E': /* even - default, if not recognized */
	case 'e':
        default: { /* Calc set bit numbers with  Brian Kernighan’s Algorithm */
            uint32_t tmp = s_uart->tr_data;
            uint8_t  nr = 0;

            while(tmp) {
              tmp &= (tmp - 1);
              nr++;
            };
	    s_uart->set_tx(nr & 1);
	    s_uart->state = S_UART_STOP_1; };
	    break;
        };
	break;
    case S_UART_STOP_1:
	    s_uart->set_tx(1); /* min 1 stop bit as 1 */
	    if (s_uart->stop == 2)
		s_uart->state = S_UART_STOP_2;
	    else
		s_uart->state = S_UART_EOT;
	break;
    case S_UART_STOP_2:
	    s_uart->set_tx(1);
	    s_uart->state = S_UART_EOT;
	break;
    case S_UART_EOT:		/* after last bit transfered */
	s_uart->state = S_UART_NONE;
        HAL_TIM_Base_Stop_IT(&htim14);
	mutex_unlock(&s_uart->mutex);
	if(s_uart->on_send_done)
	    s_uart->on_send_done();
	break;
    default:
	/* bit timer disable here, but... WTF? Why I'm here? */
        HAL_TIM_Base_Stop_IT(&htim14);
        break;
    }
}

static int uart_s_send(s_uart_t *dev, uint32_t data, void(*on_send_done)(void))
{
    int err;
    if (!dev) return E_SUART_PARM;

    err = mutex_try_lock(&dev->mutex);
    if (err != E_MTX_SUCCESS) return E_SUART_BUSY;

    dev->tr_data = data;
    dev->bits_transfered = 0;
    dev->state = S_UART_NOT_START;
    dev->on_send_done = on_send_done;
    /* bit timer enable here - make 1 bit guard interval */
    HAL_TIM_Base_Stop_IT(&htim14);
    htim14.Instance->CNT = 0; /* reset counter */
    HAL_TIM_RegisterCallback(&htim14, HAL_TIM_PERIOD_ELAPSED_CB_ID, &on_bit_time);
    HAL_TIM_Base_Start_IT(&htim14);
    return E_SUART_SUCCESS;
}

#warning TIM source clock frequency hardcoded here. M.B. calculate them?
#define TIM_CLOCK_SRC_FREQ     72000000UL

static int uart_s_init(s_uart_t *dev, int speed, uint8_t ch_bits, char par, uint8_t stop)
{
    if (!dev) return E_SUART_PARM;
    if (ch_bits < 5) return E_SUART_PARM;

    dev->bitspeed = speed;
    dev->ch_bits = ch_bits;
    dev->par = par;
    dev->stop = (stop > 2) ? 2 : stop;

    /* timer setup to call fn on_bit_time and disable them  */
    htim14.Init.Period = (TIM_CLOCK_SRC_FREQ / speed);
    htim14.Instance->CNT = 0; /* reset counter */
    HAL_TIM_Base_Init(&htim14);

    mutex_unlock(&dev->mutex);

    return E_SUART_SUCCESS;
}

/* ToDo: this is test task, in real world this functions set by init call */
static void set_tx(uint8_t level)
{
    /* UART Tx pin level inverted, if bit set - "0", if clear "1" */
    HAL_GPIO_WritePin(SUART_TX_GPIO_Port, SUART_TX_Pin,
                    level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static s_uart_t uart_dev = {
    .init = &uart_s_init,
    .send = &uart_s_send,
    .set_tx = &set_tx, /* ToDo: dynamic init required - see comment */
};
