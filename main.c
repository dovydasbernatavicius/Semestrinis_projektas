/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h" q
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include "ssd1306.h"
#include "fonts.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TRIG_PIN GPIO_PIN_6
#define TRIG_PORT GPIOA
#define ECHO_PIN GPIO_PIN_0
#define ECHO_PORT GPIOA

#define TRIG2_PIN GPIO_PIN_7
#define TRIG2_PORT GPIOA
#define ECHO2_PIN GPIO_PIN_4
#define ECHO2_PORT GPIOA


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint16_t sensor1_buffer[10] = {0};  // 10 kartu (2s / 0.2s)
uint16_t sensor2_buffer[10] = {0};
uint8_t buffer_index = 0;
uint8_t sample_count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2)
    {
        char msg[128];
        uint16_t distance1 = 0, distance2 = 0;
        uint8_t err1 = 0, err2 = 0;
        uint32_t val1, val2, pulse_width_us;
        uint32_t timeout;

      // 1 sensorius
        HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
        for (volatile int i = 0; i < 160; i++);
        HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

        timeout = HAL_GetTick();
        while (!HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) {
            if ((HAL_GetTick() - timeout) > 50) {
                err1 = 1;
                break;
            }
        }
        if (!err1) {
            val1 = __HAL_TIM_GET_COUNTER(&htim21);
            timeout = HAL_GetTick();
            while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN)) {
                if ((HAL_GetTick() - timeout) > 50) {
                    err1 = 1;
                    break;
                }
            }
            val2 = __HAL_TIM_GET_COUNTER(&htim21);
            if (!err1) {
                pulse_width_us = (val2 >= val1) ? (val2 - val1) : (0xFFFF - val1 + val2);
                distance1 = (uint16_t)((pulse_width_us * 0.0343f) / 2.0f);
                if (distance1 > 500) err1 = 1;
            }
        }

       // 2 sensorius
        HAL_GPIO_WritePin(TRIG2_PORT, TRIG2_PIN, GPIO_PIN_SET);
        for (volatile int i = 0; i < 160; i++);
        HAL_GPIO_WritePin(TRIG2_PORT, TRIG2_PIN, GPIO_PIN_RESET);

        timeout = HAL_GetTick();
        while (!HAL_GPIO_ReadPin(ECHO2_PORT, ECHO2_PIN)) {
            if ((HAL_GetTick() - timeout) > 50) {
                err2 = 1;
                break;
            }
        }
        if (!err2) {
            val1 = __HAL_TIM_GET_COUNTER(&htim21);
            timeout = HAL_GetTick();
            while (HAL_GPIO_ReadPin(ECHO2_PORT, ECHO2_PIN)) {
                if ((HAL_GetTick() - timeout) > 50) {
                    err2 = 1;
                    break;
                }
            }
            val2 = __HAL_TIM_GET_COUNTER(&htim21);
            if (!err2) {
                pulse_width_us = (val2 >= val1) ? (val2 - val1) : (0xFFFF - val1 + val2);
                distance2 = (uint16_t)((pulse_width_us * 0.03432f) / 2.0f);
                if (distance2 > 500) err2 = 1;
            }
        }

        uint16_t diff = (distance1 > distance2) ? (distance1 - distance2) : (distance2 - distance1);
        sprintf(msg, "%u %u %u\r\n", distance1, distance2, diff);
        HAL_UART_Transmit(&huart2, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);




        // bufferio loudingas
        if (!err1) sensor1_buffer[buffer_index] = distance1;
        else sensor1_buffer[buffer_index] = 0;

        if (!err2) sensor2_buffer[buffer_index] = distance2;
        else sensor2_buffer[buffer_index] = 0;

        buffer_index++;
        sample_count++;

        if (buffer_index >= 10) buffer_index = 0;

        if (sample_count >= 10)
        {
            sample_count = 0;
            uint32_t sum1 = 0, sum2 = 0;
            for (int i = 0; i < 10; i++) {
                sum1 += sensor1_buffer[i];
                sum2 += sensor2_buffer[i];
            }
            uint16_t avg1 = sum1 / 10;
            uint16_t avg2 = sum2 / 10;
            uint16_t diff = (avg1 > avg2) ? (avg1 - avg2) : (avg2 - avg1);



            // oledo displayingas
            char line[32];

            SSD1306_GotoXY(0, 0);
            SSD1306_Puts("Avg1:           ", &Font_11x18, 1);
            SSD1306_GotoXY(0, 18);
            SSD1306_Puts("Avg2:           ", &Font_11x18, 1);
            SSD1306_GotoXY(0, 36);
            SSD1306_Puts("Diff:           ", &Font_11x18, 1);

            SSD1306_GotoXY(0, 0);
            sprintf(line, "SEN1: %u cm", avg1);
            SSD1306_Puts(line, &Font_11x18, 1);

            SSD1306_GotoXY(0, 18);
            sprintf(line, "SEN2: %u cm", avg2);
            SSD1306_Puts(line, &Font_11x18, 1);

            SSD1306_GotoXY(0, 36);
            sprintf(line, "Diff: %u cm", diff);
            SSD1306_Puts(line, &Font_11x18, 1);

            SSD1306_UpdateScreen();
        }

    }
}
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_TIM21_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_Base_Start_IT(&htim2);
  HAL_TIM_Base_Start(&htim21);


  SSD1306_Init();
  SSD1306_Fill(SSD1306_COLOR_BLACK);
  SSD1306_UpdateScreen();
  SSD1306_GotoXY(0, 0);
  SSD1306_Puts("Starting...", &Font_11x18, 1);
  SSD1306_UpdateScreen();


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_I2C1;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}


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
