/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Types/bsp_system.h"
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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for KeyTask */
osThreadId_t KeyTaskHandle;
const osThreadAttr_t KeyTask_attributes = {
  .name = "KeyTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for ShowTask */
osThreadId_t ShowTaskHandle;
const osThreadAttr_t ShowTask_attributes = {
  .name = "ShowTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for CommandTask */
osThreadId_t CommandTaskHandle;
const osThreadAttr_t CommandTask_attributes = {
  .name = "CommandTask",
  .stack_size = 160 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for TrackingTask */
osThreadId_t TrackingTaskHandle;
const osThreadAttr_t TrackingTask_attributes = {
  .name = "TrackingTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for TargetQueue */
osMessageQueueId_t TargetQueueHandle;
const osMessageQueueAttr_t TargetQueue_attributes = {
  .name = "TargetQueue"
};
/* Definitions for CommandQueue */
osMessageQueueId_t CommandQueueHandle;
const osMessageQueueAttr_t CommandQueue_attributes = {
  .name = "CommandQueue"
};
/* Definitions for ErrorQueue */
osMessageQueueId_t ErrorQueueHandle;
const osMessageQueueAttr_t ErrorQueue_attributes = {
  .name = "ErrorQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartKeyTask(void *argument);
void StartShowTask(void *argument);
void StartCommandTask(void *argument);
void StartTrackingTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of TargetQueue */
  TargetQueueHandle = osMessageQueueNew (16, sizeof(TargetMessage*), &TargetQueue_attributes);

  /* creation of CommandQueue */
  CommandQueueHandle = osMessageQueueNew (16, sizeof(uint8_t), &CommandQueue_attributes);

  /* creation of ErrorQueue */
  ErrorQueueHandle = osMessageQueueNew (3, sizeof(uint8_t), &ErrorQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of KeyTask */
  KeyTaskHandle = osThreadNew(StartKeyTask, NULL, &KeyTask_attributes);

  /* creation of ShowTask */
  ShowTaskHandle = osThreadNew(StartShowTask, NULL, &ShowTask_attributes);

  /* creation of CommandTask */
  CommandTaskHandle = osThreadNew(StartCommandTask, NULL, &CommandTask_attributes);

  /* creation of TrackingTask */
  TrackingTaskHandle = osThreadNew(StartTrackingTask, NULL, &TrackingTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartKeyTask */
/**
  * @brief  Function implementing the KeyTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartKeyTask */
__weak void StartKeyTask(void *argument)
{
  /* USER CODE BEGIN StartKeyTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartKeyTask */
}

/* USER CODE BEGIN Header_StartShowTask */
/**
* @brief Function implementing the ShowTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartShowTask */
__weak void StartShowTask(void *argument)
{
  /* USER CODE BEGIN StartShowTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartShowTask */
}

/* USER CODE BEGIN Header_StartCommandTask */
/**
* @brief Function implementing the CommandTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCommandTask */
__weak void StartCommandTask(void *argument)
{
  /* USER CODE BEGIN StartCommandTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartCommandTask */
}

/* USER CODE BEGIN Header_StartTrackingTask */
/**
* @brief Function implementing the TrackingTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTrackingTask */
__weak void StartTrackingTask(void *argument)
{
  /* USER CODE BEGIN StartTrackingTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTrackingTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

