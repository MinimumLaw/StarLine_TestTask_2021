/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "rng.h"
#include "rtc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mutex.h"
#include "rbuff_s.h"
#include "uart_s.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

static mutex_t s_uart_used; /* locked while usart_s transfer data */
static char    rb_memory_block[RINGBUFF_S_MEM_SIZE];
static rbuff_s *rb = (rbuff_s *)rb_memory_block;
static volatile uint32_t random_bytes_remain;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* honeypot for detect bugs by stacktrace */
void my_bug(void)
{
  while(1);
}

/* OK, hw_init() already competed by STMicroelectronics HAL drivers */

static void sw_init(void)
{
    mutex_unlock(&s_uart_used);
    while( s_uart->init(s_uart, 115200, 8, 'n', 1) != E_SUART_SUCCESS);

    if (rbuff_s_init((void *)rb_memory_block, RINGBUFF_S_MEM_SIZE)) my_bug();
}

static void s_uart_send_next(void)
{
    int err;
    uint8_t data;
    
    err = rbuff_s_get(rb, &data);
    if (err == E_RBS_EMPTY) { 
      mutex_unlock(&s_uart_used); /* no more data for transfer */
      HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin); /* RED Toggle on empty */
      return;
    } else if (err) { /* any other error */
      my_bug();
    } else { /* s_uart->send must return SUCCESS (zero) here */
      if (s_uart->send(s_uart, data, &s_uart_send_next)) my_bug();
    };
}

/*
 * Random delay, then ramdom size array off random data 
 * put to rb, and start transfer if required
 */
void rng_ready(RNG_HandleTypeDef *hrng, uint32_t random32bit)
{        
    /* FixMe: speed optimisation required */
    /* only one RNG module on chip */
    if (hrng->Instance != RNG) my_bug();
    uint8_t data = (random32bit & 0x0000007F); /* FILTER: only LOW ASCII */
    if (data < 0x20) data=0x20;                /* FILTER: no scecial chars */
    /* rb MUST not be locked in some priority ISR handlers, so
       rbuff_s_put must allways return success (zero) */
    if (rbuff_s_put(rb, data)) my_bug();
    if(--random_bytes_remain)
      HAL_RNG_GenerateRandomNumber_IT(hrng);
}

static void producer_task(void)
{
    uint32_t time_req;
    uint32_t rng_get;
    
    /* yes, this single task in primary code cicle, so let's use blocking */

    /* random delay */
    while(HAL_RNG_GenerateRandomNumber(&hrng, &rng_get) != HAL_OK);
    rng_get %= MAX_DELAY_MS; /* 0 ... (MAX_DELAY_MS -1) */
    time_req = HAL_GetTick() + rng_get;
    while( (time_req - HAL_GetTick()) > 0); /* FixMe: on overflow */

    /* random size */
    while(HAL_RNG_GenerateRandomNumber(&hrng, &rng_get) != HAL_OK);
    rng_get %= MAX_TEMP_ARRAY_SIZE; /* 0 ... (MAX_TEMP_ARRAY_SIZE-1) */
    random_bytes_remain = rng_get;

    if(random_bytes_remain) {
        if(HAL_RNG_RegisterReadyDataCallback(&hrng, &rng_ready) != HAL_OK) my_bug();
        while(HAL_RNG_GenerateRandomNumber_IT(&hrng) != HAL_OK);
        while(random_bytes_remain);
        while(HAL_RNG_UnRegisterReadyDataCallback(&hrng) != HAL_OK);
    
        if(!mutex_try_lock(&s_uart_used)) /* s_uart ready for receive data */
            s_uart_send_next();
        else
            HAL_GPIO_TogglePin(LD4_GPIO_Port, LD4_Pin); /* GREEN toogle on non-empty */
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM14_Init();
  MX_RNG_Init();
  MX_RTC_Init();
  /* USER CODE BEGIN 2 */

  sw_init();
  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* delay, then put random size of random data to rb, start transfer if can */
    producer_task();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_HSE_DIV8;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
