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

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM5_Init(void);
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
HID_Report_u report;

typedef struct __attribute__((packed))
{
	uint8_t button;
	int8_t mouse_x;
	int8_t mouse_y;
	int8_t wheel;
} mouseHID;

mouseHID mousehid = {0, 0, 0, 0};

void updateTime() {
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


void flipBoardArrays(struct MoveState* move) {


  // coded this while not fully sober, I feel like this was dumb, but can be fixed later as it works
  uint8_t temp[8][8] = {0};

  for(int i = 0; i < 8; i++) {
    temp[7 - i][0] = move->firstPickupState[i][7];
    temp[7 - i][1] = move->firstPickupState[i][6];
    temp[7 - i][2] = move->firstPickupState[i][5];
    temp[7 - i][3] = move->firstPickupState[i][4];
    temp[7 - i][4] = move->firstPickupState[i][3];
    temp[7 - i][5] = move->firstPickupState[i][2];
    temp[7 - i][6] = move->firstPickupState[i][1];
    temp[7 - i][7] = move->firstPickupState[i][0];
    
  }

  memcpy(&move->firstPickupState, &temp, 8 * 8 * sizeof(uint8_t));

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



  //TODO: init SPI and what not
  for(int i = 0; i < 8; i++) {
    ledstate[i] = 0xA2;
  }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

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

  //initialize game
  game = (struct GameState){&player1, &player1, &player2, false, ONE_MINUTE_LIMIT, false, false, &currentMove};
  memcpy(game.previousState, previousState, 8 * 8 * sizeof(previousState[0][0]));

  //create buffer to store state of board and initialize game struct
  //TODO: might want to add back later!!!
  // char newGame[8][8] = {
  //       {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
  //       {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
  //       {0, 0, 0, 0, 0, 0, 0, 0},
  //       {0, 0, 0, 0, 0, 0, 0, 0},
  //       {0, 0, 0, 0, 0, 0, 0, 0},
  //       {0, 0, 0, 0, 0, 0, 0, 0},
  //       {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
  //       {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
  //   };

  // memcpy(&game.chessBoard, &newGame, 8 * 8 * sizeof(char));
  
  //display proper starting times for both players
  initTime(&game);

  int count = 0;
  // mousehid.mouse_y = 600;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  // For Report ID 1
  // uint8_t report1[67] = {0};
  // report1[0] = 1;
  report.report1.reportId = 1;
  report.report1.firstPickupCol = 7;
  report.report1.firstPickupRow = 7;
  int size = sizeof(report);
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
    // report1[i] = i + 1;
    report.report1.secondPickupState[i][j] = i;
    }
  }
  
  // ... fill the report ...
  USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint32_t*)&report, 67);

  HAL_Delay(500);
  // For Report ID 2
  // uint8_t report2[69] = {0};
  // report2[0] = 2;
  report.report2.reportId = 2;
  report.report2.firstPickupCol = 7;
  report.report2.firstPickupRow = 7;
  report.report2.secondPickupCol = 7;
  report.report2.secondPickupRow = 7;
  for(int i = 1; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
    // report2[i] = i - 1;
    report.report2.thirdPickupState[i][j] = i;
    }
  }
  // ... fill the report ...
  USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint32_t *) &report, 69);
  HAL_Delay(200);

    //properly update display of each player's time
    updateTime();
    // if (count % 2 == 0) {
    //   mousehid.mouse_x = 200;
    // } else {
    //   mousehid.mouse_x = -200;
    // }

    // mousehid.button = 1;
    // USBD_HID_SendReport(&hUsbDeviceFS, &mousehid, sizeof (mousehid));
    HAL_Delay (50);


    //de-assert and re-assert load pin to load values into register's D flip flops
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
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

    //calculate if move occurred and capture data related to said move
    updateMoveShit(&game);

    //if current move is finished, transmit said data to teh desktop app
    if(game.currentMove->isFinalState && game.gameStarted) {
      uint8_t numArrays = 0;
      if (game.currentMove->secondPiecePickup) {
        numArrays = 3;

      } else {
        numArrays = 2;
      }

    // char test[11] = "MovePlayed\n";
    // volatile int ret1 = CDC_Transmit_FS(test, sizeof(test));
    // HAL_Delay(20);

    // volatile int ret2 = CDC_Transmit_FS(&numArrays , 1);
    // HAL_Delay(20);

    // if(numArrays == 2) {
    //   volatile int ret3 = CDC_Transmit_FS((uint8_t *) &game.currentMove->firstPickupState , 64);
    //   HAL_Delay(20);

    //   volatile int ret4 = CDC_Transmit_FS((uint8_t *) &game.currentMove->finalState , 64);
    // } else {
    //   volatile int ret3 = CDC_Transmit_FS((uint8_t *) &game.currentMove->firstPickupState , 64);
    //   HAL_Delay(20);

    //   volatile int ret4 = CDC_Transmit_FS((uint8_t *) &game.currentMove->secondPickupState , 64);
    //   HAL_Delay(20);

    //   volatile int ret = CDC_Transmit_FS((uint8_t *) &game.currentMove->finalState , 64);
    // }

    volatile int x = 1;


      game.currentMove->firstPiecePickup = false;
      game.currentMove->secondPiecePickup = false;
      game.currentMove->isFinalState = false;
    }
    
    //TODO: remove testSend shit later once better ironed out way to send data is made
    volatile bool testSend = false;
    if (testSend) {
      uint8_t arr1[8][8] = {
            {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 1, 1, 0, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
      };
       uint8_t arr2[8][8] = {
            {1, 1, 1, 1, 1, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {1, 1, 1, 1, 0, 1, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1}
      };

       uint8_t arr3[8][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0}
      };

    uint8_t numArrays = 2;

    // char test[11] = "MovePlayed\n";
    // char newline[1] = "\n";
    // volatile int ret1 = CDC_Transmit_FS(test, sizeof(test));
    // HAL_Delay(20);

    // volatile int ret2 = CDC_Transmit_FS(&numArrays , 1);
    // HAL_Delay(20);

    // volatile int ret3 = CDC_Transmit_FS((uint8_t *) &arr3 , 64);
    // HAL_Delay(20);

    // volatile int ret4 = CDC_Transmit_FS((uint8_t *) &arr2 , 64);

    // //must be 8 so has \0 for strcmp to use
    // char test2[8] = "";
    // USBD_CDC_SetRxBuffer(&hUsbDeviceFS, &test2[0]);
    // volatile int z = USBD_CDC_ReceivePacket(&hUsbDeviceFS);
    // HAL_Delay(20);

    // if(strcmp(test2, "resend\n") == 0) {
    //   char test[11] = "MovePlayed\n";
    //   char newline[1] = "\n";
    //   volatile int ret1 = CDC_Transmit_FS(test, sizeof(test));
    //   HAL_Delay(20);

    //   volatile int ret2 = CDC_Transmit_FS(&numArrays , 1);
    //   HAL_Delay(20);

    //   volatile int ret3 = CDC_Transmit_FS((uint8_t *) &arr1 , 64);
    //   HAL_Delay(20);

    //   volatile int ret4 = CDC_Transmit_FS((uint8_t *) &arr2 , 64);
    // }



    volatile int x = 1;

    }

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
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
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
