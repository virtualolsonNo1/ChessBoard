#include "main.h"
#include "game.h"
#include "max7219.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"
#include "usb.h"
#include "usbd_def.h"
#include "usbd_customhid.h"
#include <ctype.h>


extern HIDClockModeReports clockModeReport;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern SPI_HandleTypeDef hspi1;
extern struct GameState game;
HIDClockModeReports lightReport;

uint8_t lightsOffArr[8][8] = {0};

void initTime(struct GameState* game) {
    //initialize the display with the starting time for both players
    max7219_PrintNtos(PLAYER1_MINUTES, game->player1->clock.minutes, 2);
    max7219_PrintNtos(PLAYER1_SECONDS, game->player1->clock.seconds, 2);
    max7219_PrintNtos(PLAYER2_MINUTES, game->player2->clock.minutes, 2);
    max7219_PrintNtos(PLAYER2_SECONDS, game->player2->clock.seconds, 2);
}

void changeTimeControl(struct GameState* game) {
    //change current time control to next one in list of possible time controls
    switch(game->timeControl) {
        case ONE_MINUTE_LIMIT:
            game->timeControl = TWO_MINUTE_LIMIT;
            game->player1->clock.minutes = TWO_MIN;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = TWO_MIN;
            game->player2->clock.seconds = 0;
            break;
        case TWO_MINUTE_LIMIT:
            game->timeControl = THREE_MINUTE_LIMIT;
            game->player1->clock.minutes = THREE_MIN;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = THREE_MIN;
            game->player2->clock.seconds = 0;
            break;
        case THREE_MINUTE_LIMIT:
            game->timeControl = FIVE_MINUTE_LIMIT;
            game->player1->clock.minutes = FIVE_MIN;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = FIVE_MIN;
            game->player2->clock.seconds = 0;
            break;
        case FIVE_MINUTE_LIMIT:
            game->timeControl = TEN_MINUTE_LIMIT;
            game->player1->clock.minutes = TEN_MIN;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = TEN_MIN;
            game->player2->clock.seconds = 0;
            break;
        case TEN_MINUTE_LIMIT:
            game->timeControl = THIRTY_MINUTE_LIMIT;
            game->player1->clock.minutes = THIRTY_MIN;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = THIRTY_MIN;
            game->player2->clock.seconds = 0;
            break;
        case THIRTY_MINUTE_LIMIT:
            game->timeControl = HOUR_LIMIT;
            game->player1->clock.minutes = HOUR;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = HOUR;
            game->player2->clock.seconds = 0;
            break;
        case HOUR_LIMIT:
            game->timeControl = ONE_MINUTE_LIMIT;
            game->player1->clock.minutes = 1;
            game->player1->clock.seconds = 0;
            game->player2->clock.minutes = 1;
            game->player2->clock.seconds = 0;
            break;
    }

    //update ARR register with value for new time control
    __HAL_TIM_SET_AUTORELOAD(game->player1->clock.timer, game->timeControl);
    __HAL_TIM_SET_AUTORELOAD(game->player2->clock.timer, game->timeControl);
    HAL_TIM_Base_Init(game->player1->clock.timer);
    HAL_TIM_Base_Init(game->player2->clock.timer);
    initTime(game);
    return;
}

int minutes;
int secondsRemaining;

void resetGame(struct GameState* game) {
    //TODO: reset game to previous time control
    //reset state of game's different fields
    game->timeControl = ONE_MINUTE_LIMIT;
    game->player1->clock.minutes = 1;
    game->player1->clock.seconds = 0;
    game->player2->clock.minutes = 1;
    game->player2->clock.seconds = 0;
    game->gameStarted = false;
    game->isWhiteMove = true;

    minutes = 1;
    secondsRemaining = 0;

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

      // reset binary and character state of board for potential next game
      memcpy(game->previousState, previousState, 8 * 8 * sizeof(previousState[0][0]));
      memcpy(game->previousStateChar, newGame, 8 * 8 * sizeof(previousState[0][0]));
      memset(game->currentMove->allPieceLights, 0, 64);
      memset(game->currentMove->lightState, 0, 64);
      game->currentMove->lightsOn = false;

    //reset ARR to correct value
    __HAL_TIM_SET_AUTORELOAD(game->player1->clock.timer, game->timeControl);
    __HAL_TIM_SET_AUTORELOAD(game->player2->clock.timer, game->timeControl);
    HAL_TIM_Base_Init(game->player1->clock.timer);
    HAL_TIM_Base_Init(game->player2->clock.timer);
    
    // send reset game report to desktop app
    clockModeReport.reportId = 3;
    clockModeReport.report3.reset = 255;
    USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 2);
    lightsOff();
    game->currentMove->firstPiecePickup = false;
    game->currentMove->secondPiecePickup = false;
    game->currentMove->isFinalState = false;
    game->currentMove->lightsOn = false;
    game->currentMove->pieceNewSquare = false;
    
    return;
}

void updateMoveShit(struct GameState* game) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(!game->currentMove->firstPiecePickup && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                // update square info for picked up piece
                clockModeReport.firstPickupRow = i;
                clockModeReport.firstPickupCol = j;
                game->currentMove->firstPiecePickup = true;
                
                if ((game->isWhiteMove && isupper(game->previousStateChar[i][j])) || (!game->isWhiteMove && islower(game->previousStateChar[i][j]))) {
                    lightReport.reportId = 3;
                    lightReport.report3.reset = i << 3 | j;
                    USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&lightReport, 2);
                    USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceFS);
                    
                }
                return;
                //TODO: test if this code is fixed for picking up a piece, moving it over a square, then moving it to a different square
            } else if(game->currentMove->firstPiecePickup && !game->currentMove->secondPiecePickup && !game->currentMove->isFinalState && !(i == clockModeReport.firstPickupRow && j == clockModeReport.firstPickupCol) && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                // update square info for picked up piece
                clockModeReport.report2.secondPickupRow = i;
                clockModeReport.report2.secondPickupCol = j;
                clockModeReport.report2.finalPickupCol = 8;
                clockModeReport.report2.finalPickupRow = 8;
                game->currentMove->secondPiecePickup = true;
                return;
            } else if(game->currentMove->isFinalState) {
                    bool enteredOne = false;
                // if it was a take, check to make sure piece was moved there
                if (game->currentMove->secondPiecePickup) {
                    game->currentMove->pieceNewSquare = false;
                    // TODO: FIX THIS SO THAT YOU CAN PICK UP OPPONENT PIECE FIRST!!!!!!!!!!!!!!!!!!!!!
                    if (game->currentBoardState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 1 || game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1) {
                        enteredOne = true;
                        
                    } else if (game->previousState[i][j] == 0 && game->currentBoardState[i][j] == 1) {
                        clockModeReport.report2.finalPickupRow = i;
                        clockModeReport.report2.finalPickupCol = j;
                        enteredOne = true;
                                               
                    }
                } else {
                    if (game->previousState[i][j] == 0 && game->currentBoardState[i][j] == 1) {
                            clockModeReport.report1.finalPickupRow = i;
                            clockModeReport.report1.finalPickupCol = j;
                            enteredOne = true;
                    }
                }                     
                if (enteredOne) {
                // turn off lights for potential moves for piece
                lightsOff();

                // update previous state to that of current board
                game->currentMove->lightsOn = false;
                memcpy(game->previousState, game->currentBoardState, 8 * 8 * sizeof(game->previousState[0][0]));
                memset(game->currentMove->allPieceLights, 0, 64);
                memset(game->currentMove->lightState, 0, 64);
                
                // update isWhiteMove to correctly reflect who's turn it is
                return;
                } else {
                    // TODO ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!??????????????????????????????????????????????????????????????????///////
                }
            
            // check to see if after first piece is picked up, it's put back down to instead pick up another piece for their move
            } else if (game->currentMove->firstPiecePickup && !game->currentMove->secondPiecePickup) {
                if (game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1) {
                game->currentMove->firstPiecePickup = false;
                lightsOff();
                return;
                // if piece is moved over one of it's potential moves, only light up that square and it's original spot
                } else if (game->currentBoardState[i][j] == 1 && game->currentMove->allPieceLights[i][j] == 1 && game->previousState[i][j] == 0) {
                    game->currentMove->pieceNewSquare = true;
                    game->currentMove->pieceNewRow = i;
                    game->currentMove->pieceNewCol = j;
                    memset(game->currentMove->lightState, 0, 64);
                    game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 1;
                    game->currentMove->lightState[i][j] = 1;
                    updateLights();
                } else if (game->currentMove->pieceNewSquare && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol]) {
                    // TODO: timer debouncing shit!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                }
            } 

            // TODO: MAKE IT SO WHEN PIECE SLID OR MOVED ONTO A SQUARE THAT IT'S ALLOWED TO, ONLY THAT AND STARTING SQUARE STAY LIT


        } 
    }
    
}


void convert2DArrayToBitarray(const uint8_t input[8][8], uint8_t output[8]) {
    for (int i = 0; i < 8; i++) {
        output[i] = 0;
        for (int j = 0; j < 8; j++) {
            if (input[i][j] != 0) {
                output[i] |= (1 << j);
            }
        }
    }
}


void updateReceivedLights() {
   if (game.currentMove->receivedLightData && !(game.currentMove->lightsOn)) {
        uint8_t lights[8];
        convert2DArrayToBitarray(game.currentMove->lightState, lights);       

      volatile int test = HAL_SPI_Transmit(&hspi1, (uint8_t *)lights, 8, 10000);
      while(!(SPI1->SR & 0b10)) {}

      //after transmitting LED data to shift registers, assert and de-assert load pin to display those values
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
      while(!(GPIOA->ODR & GPIO_PIN_10)) {}
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
      while((GPIOA->ODR & GPIO_PIN_10)) {}
   }
}

void updateLights() {
    uint8_t eightBitLights[8];
    convert2DArrayToBitarray(game.currentMove->lightState, eightBitLights);       

    volatile int test = HAL_SPI_Transmit(&hspi1, (uint8_t *)eightBitLights, 8, 10000);
    while(!(SPI1->SR & 0b10)) {}

    //after transmitting LED data to shift registers, assert and de-assert load pin to display those values
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    while(!(GPIOA->ODR & GPIO_PIN_10)) {}
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    while((GPIOA->ODR & GPIO_PIN_10)) {}
}


void animateInitialLights() {
    memset(game.currentMove->lightState, 0, 64);

    int rowMax = clockModeReport.firstPickupRow < 4 ? 7 - clockModeReport.firstPickupRow : clockModeReport.firstPickupRow;
    int colMax = clockModeReport.firstPickupCol < 4 ? 7 - clockModeReport.firstPickupCol : clockModeReport.firstPickupCol;
    int maxVal = (rowMax >= colMax) ? rowMax : colMax;
    for(int i = 1; i < 8; i++) {
        uint8_t eightBitLights[8] = {0};
        // check below to see if light should go on
        if (clockModeReport.firstPickupRow + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol] = 1;
        }
        // check below and to the right if light should go on
        if (clockModeReport.firstPickupRow + i <= 7 && clockModeReport.firstPickupCol + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol + i] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol + i] = 1;
        }
        // check below and to the left if light should go on
        if (clockModeReport.firstPickupRow + i <= 7 && clockModeReport.firstPickupCol - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol - i] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol - i] = 1;
        }

        // check above to see if lights should go on
        if (clockModeReport.firstPickupRow - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol] = 1;
        }
        // check above and to the right if light should go on
        if (clockModeReport.firstPickupRow - i >= 0 && clockModeReport.firstPickupCol + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol + i] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol + i] = 1;
        }
        // check above and to the left if light should go on
        if (clockModeReport.firstPickupRow - i >= 0 && clockModeReport.firstPickupCol - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol - i] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol - i] = 1;
        }

        // check to the right to see if lights should go on
        if (clockModeReport.firstPickupCol + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + i] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + i] = 1;
        }
        // check to the left to see if lights should go on
        if (clockModeReport.firstPickupCol - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - i] == 1) {
            game.currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - i] = 1;
        }
        // update lights
        updateLights();

        osDelay(100);

        // HAL_Delay(20);
    }
}


void lightsOff() {
    uint8_t lightsOff[8][8] = {0};
    memcpy(game.currentMove->lightState, lightsOffArr, 64);
    updateLights();
}

void checkStartingSquares() {
    bool lightsNeedUpdated = false;
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 8; j++) {
            if (game.currentBoardState[i][j] == 0 && game.currentMove->lightState[i][j] == 0) {
                game.currentMove->lightState[i][j] = 1;
                lightsNeedUpdated = true;
            } else if (game.currentBoardState[i][j] == 1 && game.currentMove->lightState[i][j] == 1) {
                game.currentMove->lightState[i][j] = 0;
                lightsNeedUpdated = true;
            }
        }
    }

    for(int i = 6; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if (game.currentBoardState[i][j] == 0 && game.currentMove->lightState[i][j] == 0) {
                game.currentMove->lightState[i][j] = 1;
                lightsNeedUpdated = true;
            } else if (game.currentBoardState[i][j] == 1 && game.currentMove->lightState[i][j] == 1) {
                game.currentMove->lightState[i][j] = 0;
                lightsNeedUpdated = true;
            }

        }
    }
    
    if (lightsNeedUpdated) {
        updateLights();
    }
    
}
