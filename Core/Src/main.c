/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
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
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ESP8266.h"
#include "delay.h"
#include "OLED.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "DS18B20.h"
#include "MAX30102.h"
#include "DS1302.h"
#include "MPU6050.h"
#include "GPS.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
DS1302_Time_t time;
DS1302_Time_t pressure_set_time = {0, 0, 0, 0, 0, 0, 0};
MPU6050_t MPU6050_Data;
char str[200];
float temp_value;
float jd = 119.292511, wd = 26.104894;
uint8_t heart, spo2;
uint8_t pressure, pressure_flag, pressure_time;
int16_t X, Y, Z;
uint8_t tumble, tumble_state, step, step_state;
uint8_t warning_type;
uint8_t mode;
uint16_t temp_max = 99, heart_max = 120, spo2_max = 100;
extern uint8_t time_flag;
extern uint8_t sec_tmep, minute_temp, hour_temp;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */




void Buzzer_Set_state(uint8_t state)
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, (GPIO_PinState)state);
}


/*
*	功能：控制逻辑
*	参数：无
*	返回值：无
*/
uint8_t pressure_state;
void Ctrl(void)
{
	if (mode != 2)
	{
						//	2												3											4											5
		if ((temp_value > temp_max) || (tumble == 1) || (heart > heart_max) || (spo2 > spo2_max))
		{
			warning_type =(temp_value > temp_max) ? 2 : (tumble == 1) ? 3 : (heart > heart_max) ? 4 : 5;
			Buzzer_Set_state(1);
		}
		else
		{
			warning_type = 0;
			Buzzer_Set_state(0);
		}
	}
	
	

}
/*
*	功能：页面显示
*	参数：无
*	返回值：无
*/
void Page(void)
{
	OLED_ShowChinese(1, 1, (mode == 2) ? "阈值" :  "自动");
	if (mode == 0)
	{
//	OLED_ShowSignedNum(1, 1, MPU6050_Data.gx, 4, 8);
//	OLED_ShowSignedNum(1, 16, MPU6050_Data.gy, 4, 8);
//	OLED_ShowSignedNum(1, 32, MPU6050_Data.gz, 4, 8);
//	OLED_ShowChinese(64, 48, "跌倒：");
//	OLED_ShowChinese(105, 48, (tumble == 1) ? "是" :  "否");
//	OLED_ShowChinese(1, 48, "步数：");
//	OLED_ShowNum(40, 48, step, 2, 8);
		sprintf(str, "%02d:%02d:%02d ", time.hour, time.minute, time.second);
		OLED_ShowString(40, 1, str, 8);
        OLED_ShowChinese(1, 16, "跌倒：");
		OLED_ShowChinese(40, 16, (MPU6050_Data.tumble_state == 1) ? "是" :  "否");
		OLED_ShowChinese(1, 48, "步数：");
		OLED_ShowNum(40, 48, MPU6050_Data.step_count, 2, 8);
		OLED_ShowChinese(1, 32, "温度：");
		OLED_ShowFloatNum(40, 32, temp_value, 2, 1, 8);
//		OLED_ShowChinese(1, 16, "经度：");
//		OLED_ShowChinese(1, 32, "纬度：");
//		sprintf(str, "%.6f", jd);
//		OLED_ShowString(40, 16, str, 8);
//		sprintf(str, "%.6f", wd);
//		OLED_ShowString(40, 32, str, 8);
	}
	else if(mode == 1)
	{
		OLED_ShowChinese(1, 32, "心率：");
		sprintf(str, "%d ", heart);
		OLED_ShowString(40, 32, str, 8);
		
		OLED_ShowChinese(65, 32, "血氧：");
		sprintf(str, "%d  ", spo2);
		OLED_ShowString(105, 32, str, 8);
	}
	else if (mode == 2)
	{
		OLED_ShowChinese(1, 16, "温度：");
		sprintf(str, "%d  ", temp_max);
		OLED_ShowString(40, 16, str, 8);
		OLED_ShowChinese(1, 32, "心率：");
		sprintf(str, "%d ", heart_max);
		OLED_ShowString(40, 32, str, 8);
		OLED_ShowChinese(1, 48, "血氧：");
		sprintf(str, "%d  ", spo2_max);
		OLED_ShowString(40, 48, str, 8);
	}
}


void ADXL345_IS_Tumble(void)
{
	switch(tumble_state)
	{
		case 0:
			if ((X < 200) && (Y < 400) && (Z > 800))
			{
				tumble = 0;
				tumble_state = 1;
			}
			break;
		case 1:
			if ((X > 200) && ((Y < 1000)) && (Z < 800))
			{
				tumble_state = 0;
				tumble = 1;
			}
			break;
	}
}


void ADXL345_IS_Step(void)
{
	switch(step_state)
	{
		case 0:
			if ((X < 200) && (Y < 400) && (Z > 800))
			{
				step_state = 1;
			}
			break;
		case 1:
			if ((X < 0) && (Y > 0) && (Z < 1000))
			{
				step++;
				step_state = 0;
			}
			break;
	}
}
/*
*	功能：发送数据
*	参数：无
*	返回值：无
*/
uint8_t send_time;
void Send_Data(void)
{
	if (send_time < 20)
	{
		send_time++;
		return ;
	}
	send_time = 0;
	
	sprintf(str,"{\"temp\":%.1f,\"heart\":%d,\"spo2\":%d,\"tumble\":%d,\"step\":%d,\"warning\":%d,\"jd\":%.6f,\"wd\":%.6f}\r\n", temp_value, heart, spo2, tumble, step, warning_type,jd ,wd);
	ESP8266_SendData(str);
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
  MX_USART1_UART_Init();
  MX_ADC1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  OLED_Init();
  ESP8266_Init();
	max30102_init();
	DS1302_Init();
	MPU6050_Init();
//	DS1302_Write_Time(25, 12, 22, 17, 17, 0, 1);
	GPS_Init();
	HAL_TIM_Base_Start_IT(&htim1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//	ADXL345_IS_Tumble();
//	ADXL345_ReadData(&X, &Y, &Z);
//		ADXL345_ReadData(&X, &Y, &Z);

    DS1302_ReadTime(&time);
    temp_value = DS18B20_GetTemp();
		MPU6050_Get_Step(&MPU6050_Data);
		MPU6050_is_Tumble(&MPU6050_Data);
    ESP8266_GetIPD(100);
    Send_Data();
    if(mode == 1)
    {
      MAX30102_GetData(&heart, &spo2);
    }
    GPS_Read(&jd, &wd);
    Ctrl();
    Page();
    OLED_Update();
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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
