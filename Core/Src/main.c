/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
TIM_HandleTypeDef htim1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

typedef enum
{
  Dir1 = GPIO_PIN_SET,
  Dir2 = GPIO_PIN_RESET
} Dir_t;

typedef enum
{
  MOTOR_STOP = 0,
  MOTOR_ACCEL,
  MOTOR_CRUISE,
  MOTOR_DECEL
} MotorState_t;

typedef struct
{
  volatile uint32_t Step_Goal;
  volatile uint32_t Step_Now;
  volatile MotorState_t State;

  float Start_Speed;
  float Target_Speed;
  float Current_Speed;

  uint32_t Accel_Steps;
  uint32_t Decel_Steps;

  float Accel_Rate;
  float Decel_Rate;
} StepperCtrl_t;

StepperCtrl_t motor1;

void Motor_Step_Trapezoid(float Target_Speed, uint32_t Total_Steps, uint32_t Accel_Steps, uint32_t Decel_Steps, Dir_t Dir)
{
  if (motor1.State != MOTOR_STOP)
    return;
  else
  {
    motor1.Step_Goal = Total_Steps;
    motor1.Step_Now = 0;
    motor1.Start_Speed = 100.f;
    motor1.Current_Speed = motor1.Start_Speed;
    if (Target_Speed < motor1.Start_Speed)
      motor1.Target_Speed = motor1.Start_Speed;
    else
      motor1.Target_Speed = Target_Speed;

    if (Accel_Steps + Decel_Steps >= Total_Steps)
    {
      motor1.Accel_Steps = Total_Steps / 2;
      motor1.Decel_Steps = Total_Steps - motor1.Accel_Steps;
    }
    else
    {
      motor1.Accel_Steps = Accel_Steps;
      motor1.Decel_Steps = Decel_Steps;
    }

    motor1.Accel_Rate = (motor1.Target_Speed - motor1.Start_Speed) / Accel_Steps;
    motor1.Decel_Rate = (motor1.Target_Speed - motor1.Start_Speed) / Decel_Steps;

    uint32_t ARR = (uint32_t)(1800000.0f / motor1.Current_Speed);
    __HAL_TIM_SET_AUTORELOAD(&htim1, ARR - 1);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ARR / 2);
    __HAL_TIM_SET_COUNTER(&htim1, 0);

    motor1.State = MOTOR_ACCEL;
    HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, (GPIO_PinState)Dir);
    HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
    HAL_TIM_PWM_Start_IT(&htim1, TIM_CHANNEL_1);
  }
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM1)
  {
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
      motor1.Step_Now++;
      if (motor1.Step_Now >= motor1.Step_Goal)
      {
        HAL_TIM_PWM_Stop_IT(&htim1, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);
        motor1.State = MOTOR_STOP;
      }
      else
      {
        if (motor1.Step_Now <= motor1.Accel_Steps)
        {
          motor1.State = MOTOR_ACCEL;
          motor1.Current_Speed += motor1.Accel_Rate;
        }
        else if (motor1.Step_Now > (motor1.Step_Goal - motor1.Decel_Steps))
        {
          motor1.State = MOTOR_DECEL;
          motor1.Current_Speed -= motor1.Decel_Rate;
          if (motor1.Current_Speed < motor1.Start_Speed)
            motor1.Current_Speed = motor1.Start_Speed;
        }
        else
        {
          motor1.State = MOTOR_CRUISE;
          motor1.Current_Speed = motor1.Target_Speed;
        }

        uint32_t new_ARR = (uint32_t)(1800000.0f / motor1.Current_Speed);

        // 硬件保护极限值
        if (new_ARR > 65535)
          new_ARR = 65535;
        if (new_ARR < 100)
          new_ARR = 100;

        __HAL_TIM_SET_AUTORELOAD(&htim1, new_ARR - 1);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, new_ARR / 2);
      }
    }
  }
}

// volatile uint32_t Motor_Step_Goal;
// volatile uint32_t Motor_Step_Now;
// volatile uint8_t Motor_Is_Running;

// void Motor_Step(uint32_t Step_Goal, GPIO_PinState Dir, float speed)
// {

//   if (Motor_Is_Running == 1)
//     return;
//   else
//   {
//     uint32_t freq = speed / (1.8f);
//     if (freq == 0)
//       freq = 1;
//     uint32_t ARR = 1000000 / freq;

//     if (ARR > 65535)
//       ARR = 65535;
//     if (ARR < 100)
//       ARR = 100;

//     __HAL_TIM_SET_AUTORELOAD(&htim1, ARR - 1);
//     __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ARR / 2);
//     __HAL_TIM_SET_COUNTER(&htim1, 0);

//     Motor_Step_Goal = Step_Goal;
//     Motor_Step_Now = 0;
//     Motor_Is_Running = 1;

//     HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, Dir);
//     HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_RESET);
//     HAL_TIM_PWM_Start_IT(&htim1, TIM_CHANNEL_1);
//   }
// }
// void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
// {
//   if (htim->Instance == TIM1)
//   {
//     if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
//     {
//       Motor_Step_Now++;
//       if (Motor_Step_Now >= Motor_Step_Goal)
//       {
//         HAL_TIM_PWM_Stop_IT(&htim1, TIM_CHANNEL_1);
//         HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);

//         Motor_Is_Running = 0;
//       }
//     }
//   }
// }
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
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  __HAL_TIM_SET_AUTORELOAD(&htim1, 2000 - 1);
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1000);

  uint32_t total_steps = 800;
  uint32_t acc_steps = 0.1 * total_steps;
  uint32_t dec_steps = 0.1 * total_steps;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // speed is from 30 to 6000
    Motor_Step_Trapezoid(3000, total_steps, acc_steps, dec_steps, Dir1);
    while (motor1.State != MOTOR_STOP)
      ;
    HAL_Delay(500);
    Motor_Step_Trapezoid(6000, total_steps, acc_steps, dec_steps, Dir2);
    while (motor1.State != MOTOR_STOP)
      ;
    HAL_Delay(500);

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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 72 - 1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000 - 1;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 500;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DIR_GPIO_Port, DIR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ENABLE_GPIO_Port, ENABLE_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : DIR_Pin ENABLE_Pin */
  GPIO_InitStruct.Pin = DIR_Pin | ENABLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
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

#ifdef USE_FULL_ASSERT
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
