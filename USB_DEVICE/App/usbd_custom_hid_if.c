/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_custom_hid_if.c
  * @version        : v1.0_Cube
  * @brief          : USB Device Custom HID interface file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "usbd_custom_hid_if.h"

/* USER CODE BEGIN INCLUDE */
#include "string.h"
#include "usbd_customhid.h"
#include "game.h"
#include "usb.h"
#include "cmsis_os2.h"

extern HIDClockModeReports clockModeReport;
extern osSemaphoreId_t animateLightsSem;
extern osSemaphoreId_t checkCastleSem;
extern osSemaphoreId_t checkDesktopAppErrSem;
extern osThreadId_t updateMoveTaskHandle;
extern osMessageQueueId_t errorQueueHandle;
extern struct ErrorMessage errorMessage;
extern bool waitForCastlingResponse;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim5;
bool isErrorState = false;
bool desktopError = false;
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

// Mapping piece values to characters
const char PIECE_CHARS[] = {
    [EMPTY] = 0,     // EMPTY (0)
    [W_PAWN] = 'P',  // W_PAWN (1)
    [W_KNIGHT] = 'N',// W_KNIGHT (2)
    [W_BISHOP] = 'B',// W_BISHOP (3)
    [W_ROOK] = 'R',  // W_ROOK (4)
    [W_QUEEN] = 'Q', // W_QUEEN (5)
    [W_KING] = 'K',  // W_KING (6)
    [B_PAWN] = 'p',  // B_PAWN (7)
    [B_KNIGHT] = 'n',// B_KNIGHT (8)
    [B_BISHOP] = 'b',// B_BISHOP (9)
    [B_ROOK] = 'r',  // B_ROOK (10)
    [B_QUEEN] = 'q', // B_QUEEN (11)
    [B_KING] = 'k'   // B_KING (12)
};
extern struct GameState game;


/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @brief Usb device.
  * @{
  */

/** @addtogroup USBD_CUSTOM_HID
  * @{
  */

/** @defgroup USBD_CUSTOM_HID_Private_TypesDefinitions USBD_CUSTOM_HID_Private_TypesDefinitions
  * @brief Private types.
  * @{
  */

/* USER CODE BEGIN PRIVATE_TYPES */

/* USER CODE END PRIVATE_TYPES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Defines USBD_CUSTOM_HID_Private_Defines
  * @brief Private defines.
  * @{
  */

/* USER CODE BEGIN PRIVATE_DEFINES */
#define PIECES_REPORT_LEN 32
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Macros USBD_CUSTOM_HID_Private_Macros
  * @brief Private macros.
  * @{
  */

/* USER CODE BEGIN PRIVATE_MACRO */

/* USER CODE END PRIVATE_MACRO */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_Variables USBD_CUSTOM_HID_Private_Variables
  * @brief Private variables.
  * @{
  */

/** Usb HID report descriptor. */
__ALIGN_BEGIN static uint8_t CUSTOM_HID_ReportDesc_FS[USBD_CUSTOM_HID_REPORT_DESC_SIZE] __ALIGN_END =
{
  /* USER CODE BEGIN 0 */
  0x06, 0x00, 0xFF,  // Usage Page (Vendor Defined 0xFF00)
  0x09, 0x01,        // Usage (0x01)
  0xA1, 0x01,        // Collection (Application)
  // Report ID 1: 1 set of pickup data + finalStateRow and finalStateCol
  0x85, 0x01,        //   Report ID (1)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x07,        //   Logical Maximum (7)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x04,        //   Report Count (4)
  0x09, 0x02,        //   Usage (0x02 - firstPickupRow)
  0x09, 0x03,        //   Usage (0x03 - firstPickupCol)
  0x09, 0x04,        //   Usage (0x04 - finalStateRow)
  0x09, 0x05,        //   Usage (0x05 - finalStateCol)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Report ID 2: 2 sets of pickup data + finalStateRow and finalStateCol
  0x85, 0x02,        //   Report ID (2)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x07,        //   Logical Maximum (7)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x06,        //   Report Count (6)
  0x09, 0x02,        //   Usage (0x02 - firstPickupRow)
  0x09, 0x03,        //   Usage (0x03 - firstPickupCol)
  0x09, 0x06,        //   Usage (0x06 - secondPickupRow)
  0x09, 0x07,        //   Usage (0x07 - secondPickupCol)
  0x09, 0x04,        //   Usage (0x04 - finalStateRow)
  0x09, 0x05,        //   Usage (0x05 - finalStateCol)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Report ID 3: resetGame (1 byte data)
  0x85, 0x03,        //   Report ID (3)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (2)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x01,        //   Report Count (1)
  0x09, 0x08,        //   Usage (0x08 - resetGame)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Report ID 4: 8-byte array for light status (Output)
  0x85, 0x04,        //   Report ID (4)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x08,        //   Report Count (8)
  0x09, 0x09,        //   Usage (0x09 - Light Status)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Report ID 5: 32-byte array for letters/numbers (Output)
  0x85, 0x05,        //   Report ID (5)
  0x15, 0x00,        //   Logical Minimum (0 (two empty squares))
  0x25, 0xCB,        //   Logical Maximum (203 (11001011 for Black King and Black Queen next to each other))
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x20,        //   Report Count (32)
  0x09, 0x0A,        //   Usage (0x0A - Chess Pieces array)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Report ID 6: 1-byte error message (Output)
  0x85, 0x06,        //   Report ID (6)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x01,        //   Report Count (1)
  0x09, 0x0B,        //   Usage (0x0B - Error Message)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Frontend Data
  0x85, 0x07,        //   Report ID (7)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x3,        //   Report Count (32? whatever needed for error state)
  0x09, 0x0C,        //   Usage (0x0C - Piece Current location, second piece picked up, back to original position, etc)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  // Frontend Data Error State
  0x85, 0x08,        //   Report ID (8)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0xFF,        //   Logical Maximum (255)
  0x75, 0x08,        //   Report Size (8 bits)
  0x95, 0x8,        //   Report Count (8)
  0x09, 0x0D,        //   Usage (0x0D - locations of error pieces)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  
  /* USER CODE END 0 */
  0xC0    /*     END_COLLECTION	             */
};

/* USER CODE BEGIN PRIVATE_VARIABLES */

/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Exported_Variables USBD_CUSTOM_HID_Exported_Variables
  * @brief Public variables.
  * @{
  */
extern USBD_HandleTypeDef hUsbDeviceFS;

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */
/**
  * @}
  */

/** @defgroup USBD_CUSTOM_HID_Private_FunctionPrototypes USBD_CUSTOM_HID_Private_FunctionPrototypes
  * @brief Private functions declaration.
  * @{
  */

static int8_t CUSTOM_HID_Init_FS(void);
static int8_t CUSTOM_HID_DeInit_FS(void);
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state);

/**
  * @}
  */

USBD_CUSTOM_HID_ItfTypeDef USBD_CustomHID_fops_FS =
{
  CUSTOM_HID_ReportDesc_FS,
  CUSTOM_HID_Init_FS,
  CUSTOM_HID_DeInit_FS,
  CUSTOM_HID_OutEvent_FS
};

/** @defgroup USBD_CUSTOM_HID_Private_Functions USBD_CUSTOM_HID_Private_Functions
  * @brief Private functions.
  * @{
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Initializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_Init_FS(void)
{
  /* USER CODE BEGIN 4 */
  return (USBD_OK);
  /* USER CODE END 4 */
}

/**
  * @brief  DeInitializes the CUSTOM HID media low layer
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_DeInit_FS(void)
{
  /* USER CODE BEGIN 5 */
  return (USBD_OK);
  /* USER CODE END 5 */
}


void convert1DArrayTo2DArray(uint8_t *input, uint8_t output[8][8]) {
    int index = 0;
    
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            output[i][j] = input[index++];
        }
    }
}



/**
  * @brief  Manage the CUSTOM HID class events
  * @param  event_idx: Event index
  * @param  state: Event state
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
static int8_t CUSTOM_HID_OutEvent_FS(uint8_t event_idx, uint8_t state)
{
  /* USER CODE BEGIN 6 */
  UNUSED(event_idx);
  UNUSED(state);

  /* Start next USB packet transfer once data processing is completed */
  if (USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceFS) != (uint8_t)USBD_OK)
  {
    return -1;
  }
  
   if (event_idx == PIECES_DATA_REPORT_OUT) {
    volatile uint8_t test[PIECES_REPORT_LEN] = {0};
    uint8_t stateCharNums[64] = {0};
    memcpy(test, hUsbDeviceFS.pClassData + 1, PIECES_REPORT_LEN);

    // loop through received pieces data, converting each byte to the respective two characters it represents
    for(int i = 0; i < PIECES_REPORT_LEN; i++) {
      // 4 MSbits are first piece (earlier index), 4 LSbits are second piece (second index)
      stateCharNums[i * 2] = PIECE_CHARS[test[i] >> SECOND_PIECE_BIT_SHIFT];        
      stateCharNums[(i * 2) + 1] = PIECE_CHARS[test[i] & FIRST_PIECE_BITS];        
    }

    convert1DArrayTo2DArray(stateCharNums, game.previousStateChar);
    // if received pieces data, no error occurred on the desktop app end so can continue accordingly
    osSemaphoreRelease(checkDesktopAppErrSem);

  // if error received from desktop app, replay the move as button hit too early or for wrong move DESPITE ALL THAT ERROR HANDLING cause people are dumb ig
  } else if (event_idx == ERROR_REPORT_OUT) {
    uint8_t errorStatus;
    memcpy(&errorStatus, hUsbDeviceFS.pClassData + 1, 1);
    // check if error or checkmate occurred
    if (errorStatus == 255) {
    isErrorState = true;
    errorMessage.numPieces = 1;
    errorMessage.resetState = NO_PIECE_PICKUP;
    desktopError = true;
    return 0;

    // white checkmate
    } else if (errorStatus == 1) {
    game.resetNow = true;

    // black checkmate
    } else if (errorStatus == 2) {
    game.resetNow = true;

    // stalemate or insufficient material
    } else if (errorStatus == 3) {
    game.resetNow = true;

    } else {
      // ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      volatile int x = 1;
    }
    osSemaphoreRelease(checkDesktopAppErrSem);
    return 0;

  // if the report ID is 4 for lights data, convert from 8 byte array to 8x8 2D array and make sure there are possible moves for the piece
  }  else if (event_idx == LIGHTS_DATA_REPORT_OUT) {
    uint8_t test[8] = {0};
    memcpy(test, hUsbDeviceFS.pClassData + 1, sizeof(test));

    // convert 8 byte array to 8x8 2d array
    for(int i = 0; i < 8; i++) {
      game.currentMove->lightState[i][0] = (0b10000000 & test[i]) >> 7; 
      game.currentMove->lightState[i][1] = (0b01000000 & test[i]) >> 6; 
      game.currentMove->lightState[i][2] = (0b00100000 & test[i]) >> 5; 
      game.currentMove->lightState[i][3] = (0b00010000 & test[i]) >> 4; 
      game.currentMove->lightState[i][4] = (0b00001000 & test[i]) >> 3; 
      game.currentMove->lightState[i][5] = (0b00000100 & test[i]) >> 2; 
      game.currentMove->lightState[i][6] = (0b00000010 & test[i]) >> 1; 
      game.currentMove->lightState[i][7] = (0b00000001 & test[i]) >> 0; 
    }

    memcpy(game.currentMove->allPieceLights, game.currentMove->lightState, 64);
    game.currentMove->receivedLightData = true;
    
    bool possibleMove = false;
    
    // check for possible moves
    for(int i = 0; i < 8; i++) {
      for(int j = 0; j < 8; j++) {
        if (game.currentMove->allPieceLights[i][j] == 1) {
          possibleMove = true;
          break;
        } 
      }
    }
    
    // if there are no possible moves, set error state to true
    if (!possibleMove) {
      // set isErrorState to true so update move thread can suspend itself later and start blink error task
      isErrorState = true;
      errorMessage.numPieces = 1;
      errorMessage.resetState = NO_PIECE_PICKUP;
      errorMessage.firstPickupRow = clockModeReport.firstPickupRow;
      errorMessage.firstPickupCol = clockModeReport.firstPickupCol;
      if (waitForCastlingResponse) {
        osSemaphoreRelease(checkCastleSem);
        return 0;
      }
    } else {
      if (waitForCastlingResponse) {
        osSemaphoreRelease(checkCastleSem);
        return 0;
      }
      // if received light data and it's the player's peice that was picked up, if it's not a knight animate lights, otherwise just light up squares that can take piece or squares for a knight's moves
      if (game.currentMove->firstPiecePlayersColor && game.previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] != 'n' && game.previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] != 'N') {
        osSemaphoreRelease(animateLightsSem);
      } else {
        updateReceivedLights();
      }
    }
  }
  
  return (USBD_OK);
  /* USER CODE END 6 */
}

/* USER CODE BEGIN 7 */
/**
  * @brief  Send the report to the Host
  * @param  report: The report to be sent
  * @param  len: The report length
  * @retval USBD_OK if all operations are OK else USBD_FAIL
  */
/*
static int8_t USBD_CUSTOM_HID_SendReport_FS(uint8_t *report, uint16_t len)
{
  return USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, report, len);
}
*/
/* USER CODE END 7 */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_IMPLEMENTATION */

/* USER CODE END PRIVATE_FUNCTIONS_IMPLEMENTATION */
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

