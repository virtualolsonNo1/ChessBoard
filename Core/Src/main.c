/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "cmsis_os.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "game.h"
#include "max7219.h"
#include "stm32f4xx_hal.h"
// #include "usbd_cdc.h"
#include "usb_device.h"
// #include "usbd_cdc_if.h"
#include "usbd_def.h"
#include "string.h"
#include "usbd_customhid.h"
#include "usb.h"
#include "test.h"

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
SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim5;

/* Definitions for blinkErrorTask */
osThreadId_t blinkErrorTaskHandle;
const osThreadAttr_t blinkErrorTask_attributes = {
  .name = "blinkErrorTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for updateTimeTask */
osThreadId_t updateTimeTaskHandle;
const osThreadAttr_t updateTimeTask_attributes = {
  .name = "updateTimeTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for updateMoveTask */
osThreadId_t updateMoveTaskHandle;
const osThreadAttr_t updateMoveTask_attributes = {
  .name = "updateMoveTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for animLightsTask */
osThreadId_t animLightsTaskHandle;
const osThreadAttr_t animLightsTask_attributes = {
  .name = "animLightsTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for animateMutex */
osMutexId_t animateMutexHandle;
const osMutexAttr_t animateMutex_attributes = {
  .name = "animateMutex"
};
/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
extern uint8_t lightsOffArr[8][8];
osSemaphoreId_t animateLightsMutex;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM5_Init(void);
void blinkError(void *argument);
void updateTime(void *argument);
void updateMove(void *argument);
void animateLights(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t boardstate[8];
uint8_t boardstateArr[8][8];
uint8_t ledstate[8];
struct Clock clock1;
struct Clock clock2;
struct Player player1;
struct Player player2;
struct MoveState currentMove;
struct GameState game;
// typedef struct report1 firstReport;
// typedef struct report2 secondReport;
HIDClockModeReports clockModeReport;


void updateTimeOld() {
  //grab count value from CNT register of the active player's timer
    int count = game.activePlayer->clock.timer->Instance->CNT;

    //convert count to minutes and seconds
    volatile int totalSeconds = count / 1000;
    int minutes = totalSeconds / 60;
    int secondsRemaining = totalSeconds - (minutes * 60);

    uint8_t minutesReg;
    uint8_t secondsReg;

    //use correct register values for display depending on who is the active player
    if(game.activePlayer == game.player1) {
            minutesReg = PLAYER1_MINUTES;
            secondsReg = PLAYER1_SECONDS;
        } else {
            minutesReg = PLAYER2_MINUTES;
            secondsReg = PLAYER2_SECONDS;
        }

    //if the current player's clock has different time than is displayed, update the display accordingly
    if(secondsRemaining != game.activePlayer->clock.seconds || minutes != game.activePlayer->clock.minutes) {
        game.activePlayer->clock.seconds = secondsRemaining;
        game.activePlayer->clock.minutes = minutes;
        max7219_PrintNtos(minutesReg, minutes, 2);
        max7219_PrintNtos(secondsReg, secondsRemaining, 2);
    } 

    //if the game needs reset, properly do so
    //TODO: this makes no logical sense to have in time shit, move to better spot later!!!!!!!
    if (game.resetNow) {
        game.resetNow = false;
        resetGame(&game);
        initTime(&game);
    }
}


// TODO: MAKE THSI SHIT!!!!!!!!
// void flipBoardArrays(struct MoveState* move) {
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
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_TIM2_Init();
  MX_TIM5_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  //properly set up 7 segment LCD
  max7219_Init(0xA);
  max7219_Decode_On();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);


  const osMutexAttr_t myMutex_attributes = {
    .name = "animateLightsMutex"
  };
  animateLightsMutex = osSemaphoreNew(1, 0, animateLightsMutex);

  //TODO: init SPI and what not
  for(int i = 0; i < 8; i++) {
    ledstate[i] = 0xA2;
  }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

  //initialize clocks properly, enabling proper interrupts
  TIM2->SR &= ~(TIM_SR_UIF_Msk);
  TIM2->DIER |= 1;

  TIM5->SR &= ~(TIM_SR_UIF_Msk);
  TIM5->DIER |= 1;

  //instantiate and properly initialize both players' clocks
  clock1 = (struct Clock){&htim2, 1, 0};
  clock2 = (struct Clock){&htim5, 1, 0};

  //instantiate and properly initialize both players
  player1 = (struct Player){clock1, true};
  player2 = (struct Player){clock2, false};

  //initialize currentMove
  currentMove = (struct MoveState){false, false, false};

  uint8_t previousState[8][8] = {
            {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
      };
    // create buffer to store state of board and initialize game struct
    char newGame[8][8] = {
          {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
          {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
          {0, 0, 0, 0, 0, 0, 0, 0},
          {0, 0, 0, 0, 0, 0, 0, 0},
          {0, 0, 0, 0, 0, 0, 0, 0},
          {0, 0, 0, 0, 0, 0, 0, 0},
          {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
          {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
      };

  //initialize game
  game = (struct GameState){&player1, &player1, &player2, false, ONE_MINUTE_LIMIT, false, false, &currentMove};
  memcpy(game.previousState, previousState, 8 * 8 * sizeof(previousState[0][0]));
  memcpy(game.previousStateChar, newGame, 8 * 8 * sizeof(newGame[0][0]));


  // memcpy(&game.chessBoard, &newGame, 8 * 8 * sizeof(char));
  
  //display proper starting times for both players
  initTime(&game);

  int count = 0;
  lightsOff();
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of animateMutex */
  animateMutexHandle = osMutexNew(&animateMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of blinkErrorTask */
  // blinkErrorTaskHandle = osThreadNew(blinkError, NULL, &blinkErrorTask_attributes);

  /* creation of updateTimeTask */
  // updateTimeTaskHandle = osThreadNew(updateTime, NULL, &updateTimeTask_attributes);

  /* creation of updateMoveTask */
  updateMoveTaskHandle = osThreadNew(updateMove, NULL, &updateMoveTask_attributes);

  /* creation of animLightsTask */
  animLightsTaskHandle = osThreadNew(animateLights, NULL, &animLightsTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  // For Report ID 1
  // uint8_t report1[5] = {0, 1, 2, 3, 4};
  // report1[0] = 1;
  // clockModeReport.reportId = 1;
  // clockModeReport.firstPickupRow = 2;
  // clockModeReport.firstPickupCol = 3;
  // clockModeReport.report1.finalPickupRow = 4;
  // clockModeReport.report1.finalPickupCol = 5;
  // int size = sizeof(clockModeReport);
  // for(int i = 0; i < 8; i++) {
  //   for(int j = 0; j < 8; j++) {
  //   // report1[i] = i + 1;
  //   clockModeReport.secondPickupState[i][j] = i;
  //   }
  // }
  
  // ... fill the report ...
  // USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report1, 5);

  // HAL_Delay(500);
  // For Report ID 2
  // uint8_t report2[7] = {0, 1, 2, 3, 4, 5, 6};
  // report2[0] = 2;
  // clockModeReport.reportId = 2;
  // clockModeReport.firstPickupRow = 2;
  // clockModeReport.firstPickupCol = 3;
  // clockModeReport.report2.secondPickupRow = 4;
  // clockModeReport.report2.secondPickupCol = 5;
  // clockModeReport.report2.finalPickupRow = 6;
  // clockModeReport.report2.finalPickupCol = 7;
  // for(int i = 1; i < 8; i++) {
  //   for(int j = 0; j < 8; j++) {
  //   // report2[i] = i - 1;
  //   clockModeReport.thirdPickupState[i][j] = i;
  //   }
  // }
  // ... fill the report ...
  // USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,  report2, 7);
  // HAL_Delay(200);

    //properly update display of each player's time
    // updateTimeOld();
    // if (count % 2 == 0) {
    //   mousehid.mouse_x = 200;
    // } else {
    //   mousehid.mouse_x = -200;
    // }

    // mousehid.button = 1;
    // USBD_HID_SendReport(&hUsbDeviceFS, &mousehid, sizeof (mousehid));
    HAL_Delay (10);


    // //de-assert and re-assert load pin to load values into register's D flip flops
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    // HAL_Delay(1);
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

    // //transmit MISO data from shift registers into boardstate buffer
    // HAL_SPI_Receive(&hspi1, (uint8_t *)boardstate, 8, 100000);

    // //turn received boardstate into 2d array instead of 1d array of uint8_t's
    // while((SPI1->SR & 0b1)) {}
    
    // //constatntly store state of board in game so can be used when button is pressed
    // for(int i = 0; i < 8; i++) {
    //   game.currentBoardState[i][0] = (0b10000000 & ~boardstate[i]) >> 7; 
    //   game.currentBoardState[i][1] = (0b01000000 & ~boardstate[i]) >> 6; 
    //   game.currentBoardState[i][2] = (0b00100000 & ~boardstate[i]) >> 5; 
    //   game.currentBoardState[i][3] = (0b00010000 & ~boardstate[i]) >> 4; 
    //   game.currentBoardState[i][4] = (0b00001000 & ~boardstate[i]) >> 3; 
    //   game.currentBoardState[i][5] = (0b00000100 & ~boardstate[i]) >> 2; 
    //   game.currentBoardState[i][6] = (0b00000010 & ~boardstate[i]) >> 1; 
    //   game.currentBoardState[i][7] = (0b00000001 & ~boardstate[i]) >> 0; 

    // }

    // TODO: remove this later once hall effect sensors fixed!!!!!!!!!!!!!!!!!!!!!!!!!
    // TODO: FIGURE OUT WHY ALL THESE ARE FUCKED!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // game.currentBoardState[6][1] = 1;
    // game.currentBoardState[7][1] = 1;
    // game.currentBoardState[3][2] = 0;

    volatile int x = 8;


  //TODO: READD LED CODE ONCE THEY'RE WORKING! Will eventually need to get data back from app with what squares to light up when piece picked up
    //convert hall data to LED order (as shift register wired up backwards for hall vs LED)
    // boardstateToLed(&boardstate, &ledstate);

    // //send buffer to shift registers before sending their values to LEDs
    //  volatile int test = HAL_SPI_Transmit(&hspi1, (uint8_t *)ledstate, 8, 10000);
    // while(!(SPI1->SR & 0b10)) {}

    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    // while(!(GPIOA->ODR & GPIO_PIN_10)) {}
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    // while((GPIOA->ODR & GPIO_PIN_10)) {}
    
    // volatile int y = 8;

    // loop through to test LED's in binary fashion, testing all possible combinations
    // for(int i = 0; i < 256; i++) {
    //   for(int j = 0; j < 8; j++) {
    //     ledstate[j] = i;
    //   }
    //   volatile int test = HAL_SPI_Transmit(&hspi1, (uint8_t *)ledstate, 8, 10000);
    //   while(!(SPI1->SR & 0b10)) {}

    //   //after transmitting LED data to shift registers, assert and de-assert load pin to display those values
    //   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    //   while(!(GPIOA->ODR & GPIO_PIN_10)) {}
    //   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    //   while((GPIOA->ODR & GPIO_PIN_10)) {}
    // }

    //TODO: update to send better data later

    // if (game.gameStarted) {
    // //calculate if move occurred and capture data related to said move
    // updateMoveShit(&game);
    // } else {
    //   // light up squares where pieces aren't but should be before start of game
    //   checkStartingSquares();
    // }

    // //if current move is finished, transmit said data to teh desktop app
    // if(game.currentMove->isFinalState && game.gameStarted) {
      
    //   if (game.currentMove->secondPiecePickup) {
        
    //     clockModeReport.reportId = 2;
    //     USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint32_t*)&clockModeReport, 7);
    //   } else {
    //     clockModeReport.reportId = 1;

    //     USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint32_t*)&clockModeReport, 5);
    //   }

    // volatile int x = 1;
    
    // // TODO: MAKE REPORT TO BE SENT BACK HERE WITH CHAR BOARD POSITION!!!!!!!!!!!!!!!!!!!!

    
    //   if (clockModeReport.reportId == 2) {
    //     clockModeReport.report2.finalPickupRow = 8;
    //     clockModeReport.report2.finalPickupCol = 8;
    //   } else {
    //     clockModeReport.report1.finalPickupRow = 8;
    //     clockModeReport.report1.finalPickupCol = 8;
        
    //   }
    //   game.currentMove->firstPiecePickup = false;
    //   game.currentMove->secondPiecePickup = false;
    //   game.currentMove->isFinalState = false;
    //   game.currentMove->lightsOn = false;
    //   game.currentMove->pieceNewSquare = false;
    // }
    

    // volatile bool sendTest = false;
    // if (sendTest) {
    //   HAL_Delay(1000);
    //   sendTestGame();

    //   HAL_Delay(1000);
    //   lightsOff();

    // }
    
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
  RCC_OscInitStruct.PLL.PLLM = 6;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV6;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 16000;
  htim2.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  htim2.Init.Period = 60000;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM5_Init(void)
{

  /* USER CODE BEGIN TIM5_Init 0 */

  /* USER CODE END TIM5_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM5_Init 1 */

  /* USER CODE END TIM5_Init 1 */
  htim5.Instance = TIM5;
  htim5.Init.Prescaler = 16000;
  htim5.Init.CounterMode = TIM_COUNTERMODE_DOWN;
  htim5.Init.Period = 60000;
  htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim5.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim5, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim5, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM5_Init 2 */

  /* USER CODE END TIM5_Init 2 */

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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4|GPIO_PIN_9|GPIO_PIN_10, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA1 PA2 PA3 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA4 PA9 PA10 */
  GPIO_InitStruct.Pin = GPIO_PIN_4|GPIO_PIN_9|GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void boardstateToLed() {
  for(int i = 0; i < 8; i++) {
    int ledtemp = 0;
    int ledbit = 0x1;
    int boardbit = 0x80;
    for(int j = 0; j < 8; j++) {
      if(boardbit & boardstate[i]) {
        ledtemp |= ledbit;
      }
      boardbit = boardbit >> 1;
      ledbit = ledbit << 1;
    }
    ledstate[i] = ledtemp;
  }
}
/* USER CODE END 4 */

/* USER CODE BEGIN Header_blinkError */
/**
  * @brief  Function implementing the blinkErrorTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_blinkError */
void blinkError(void *argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(portMAX_DELAY);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_updateTime */
/**
* @brief Function implementing the updateTimeTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_updateTime */
void updateTime(void *argument)
{
  /* USER CODE BEGIN updateTime */
  /* Infinite loop */
  for(;;)
  {
    updateTimeOld();
    osDelay(1);
  }
  /* USER CODE END updateTime */
}

/* USER CODE BEGIN Header_updateMove */
/**
* @brief Function implementing the updateMoveTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_updateMove */
void updateMove(void *argument)
{
  /* USER CODE BEGIN updateMove */
  /* Infinite loop */
  for(;;)
  {
    //de-assert and re-assert load pin to load values into register's D flip flops
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
    osDelay(1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);

    //transmit MISO data from shift registers into boardstate buffer
    HAL_SPI_Receive(&hspi1, (uint8_t *)boardstate, 8, 100000);

    //turn received boardstate into 2d array instead of 1d array of uint8_t's
    while((SPI1->SR & 0b1)) {}
    
    //constatntly store state of board in game so can be used when button is pressed
    for(int i = 0; i < 8; i++) {
      game.currentBoardState[i][0] = (0b10000000 & ~boardstate[i]) >> 7; 
      game.currentBoardState[i][1] = (0b01000000 & ~boardstate[i]) >> 6; 
      game.currentBoardState[i][2] = (0b00100000 & ~boardstate[i]) >> 5; 
      game.currentBoardState[i][3] = (0b00010000 & ~boardstate[i]) >> 4; 
      game.currentBoardState[i][4] = (0b00001000 & ~boardstate[i]) >> 3; 
      game.currentBoardState[i][5] = (0b00000100 & ~boardstate[i]) >> 2; 
      game.currentBoardState[i][6] = (0b00000010 & ~boardstate[i]) >> 1; 
      game.currentBoardState[i][7] = (0b00000001 & ~boardstate[i]) >> 0; 

    }

    updateTimeOld();

    if (game.gameStarted) {
      //calculate if move occurred and capture data related to said move
      updateMoveShit(&game);
    } else {
      // light up squares where pieces aren't but should be before start of game
      checkStartingSquares();
    }

    //if current move is finished, transmit said data to teh desktop app
    if(game.currentMove->isFinalState && game.gameStarted) {
      updateMoveShit(&game);
      
      if (game.currentMove->pickupState == SECOND_PIECE_PICKUP) {
        
        clockModeReport.reportId = 2;
        USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint32_t*)&clockModeReport, 7);
      } else {
        clockModeReport.reportId = 1;

        USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint32_t*)&clockModeReport, 5);
      }

    volatile int x = 1;

    osDelay(1);
    
    // TODO: MAKE REPORT TO BE SENT BACK HERE WITH CHAR BOARD POSITION!!!!!!!!!!!!!!!!!!!!

    
      if (clockModeReport.reportId == 2) {
        clockModeReport.report2.finalPickupRow = 8;
        clockModeReport.report2.finalPickupCol = 8;
      } else {
        clockModeReport.report1.finalPickupRow = 8;
        clockModeReport.report1.finalPickupCol = 8;
        
      }
      game.currentMove->pickupState = NO_PIECE_PICKUP;
      game.currentMove->isFinalState = false;
      game.currentMove->lightsOn = false;
      game.currentMove->pieceNewSquare = false;
    }
    osDelay(1);
  }
  /* USER CODE END updateMove */
}

/* USER CODE BEGIN Header_animateLights */
/**
* @brief Function implementing the animLightsTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_animateLights */
void animateLights(void *argument)
{
  /* USER CODE BEGIN animateLights */
  /* Infinite loop */
  for(;;)
  {
    osSemaphoreAcquire(animateLightsMutex, portMAX_DELAY);
    animateInitialLights();
    osDelay(1);
  }
  /* USER CODE END animateLights */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
