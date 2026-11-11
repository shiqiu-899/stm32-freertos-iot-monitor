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
#include "stdio.h"
#include "app_tasks.h"
#include "queue.h"
#include "semphr.h"
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
osThreadId defaultTaskHandle;
osThreadId SensorHandle;
osThreadId DisplayHandle;
osThreadId ControlHandle;
osThreadId CommHandle;
osThreadId LogHandle;
osMessageQId queue_sensorHandle;
osMessageQId queue_alarmHandle;
osMutexId sem_i2cHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void vTaskSensorCollect(void const * argument);
void vTaskDisplay(void const * argument);
void vTaskControl(void const * argument);
void vTaskCommunication(void const * argument);
void vTaskLog(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* definition and creation of sem_i2c */
  osMutexDef(sem_i2c);
  sem_i2cHandle = osMutexCreate(osMutex(sem_i2c));

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
  /* definition and creation of queue_sensor */
  osMessageQDef(queue_sensor, 1, SensorData_t);
  queue_sensorHandle = osMessageCreate(osMessageQ(queue_sensor), NULL);

  /* definition and creation of queue_alarm */
  osMessageQDef(queue_alarm, 5, AlarmEvent_t);
  queue_alarmHandle = osMessageCreate(osMessageQ(queue_alarm), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of Sensor */
  osThreadDef(Sensor, vTaskSensorCollect, osPriorityBelowNormal, 0, 256);
  SensorHandle = osThreadCreate(osThread(Sensor), NULL);

  /* definition and creation of Display */
  osThreadDef(Display, vTaskDisplay, osPriorityLow, 0, 256);
  DisplayHandle = osThreadCreate(osThread(Display), NULL);

  /* definition and creation of Control */
  osThreadDef(Control, vTaskControl, osPriorityNormal, 0, 256);
  ControlHandle = osThreadCreate(osThread(Control), NULL);

  /* definition and creation of Comm */
  osThreadDef(Comm, vTaskCommunication, osPriorityBelowNormal, 0, 512);
  CommHandle = osThreadCreate(osThread(Comm), NULL);

  /* definition and creation of Log */
  osThreadDef(Log, vTaskLog, osPriorityLow, 0, 256);
  LogHandle = osThreadCreate(osThread(Log), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  for(;;)
  {
    printf("[HB] tick=%lu heap_free=%u\r\n",
           (unsigned long)xTaskGetTickCount(),
           (unsigned)xPortGetFreeHeapSize());
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_vTaskSensorCollect */
/**
* @brief Function implementing the Sensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vTaskSensorCollect */
__weak void vTaskSensorCollect(void const * argument)
{
  /* USER CODE BEGIN vTaskSensorCollect */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vTaskSensorCollect */
}

/* USER CODE BEGIN Header_vTaskDisplay */
/**
* @brief Function implementing the Display thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vTaskDisplay */
__weak void vTaskDisplay(void const * argument)
{
  /* USER CODE BEGIN vTaskDisplay */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vTaskDisplay */
}

/* USER CODE BEGIN Header_vTaskControl */
/**
* @brief Function implementing the Control thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vTaskControl */
__weak void vTaskControl(void const * argument)
{
  /* USER CODE BEGIN vTaskControl */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vTaskControl */
}

/* USER CODE BEGIN Header_vTaskCommunication */
/**
* @brief Function implementing the Comm thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vTaskCommunication */
__weak void vTaskCommunication(void const * argument)
{
  /* USER CODE BEGIN vTaskCommunication */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vTaskCommunication */
}

/* USER CODE BEGIN Header_vTaskLog */
/**
* @brief Function implementing the Log thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vTaskLog */
__weak void vTaskLog(void const * argument)
{
  /* USER CODE BEGIN vTaskLog */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END vTaskLog */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

