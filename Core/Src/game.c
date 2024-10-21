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
    game->currentMove->pickupState = NO_PIECE_PICKUP;
    game->currentMove->isFinalState = false;
    game->currentMove->pieceNewSquare = false;
    
    return;
}

void updateMoveShit(struct GameState* game) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(game->currentMove->pickupState == NO_PIECE_PICKUP) {
                if (game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                    // update square info for picked up piece
                    clockModeReport.firstPickupRow = i;
                    clockModeReport.firstPickupCol = j;
                    game->currentMove->pickupState = FIRST_PIECE_PICKUP;
                    
                    if ((game->isWhiteMove && isupper(game->previousStateChar[i][j])) || (!game->isWhiteMove && islower(game->previousStateChar[i][j]))) {
                        game->currentMove->firstPiecePlayersColor = true;
                        lightReport.reportId = 3;
                        lightReport.report3.reset = i << 3 | j;
                        USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&lightReport, 2);
                        USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceFS);
                        
                    } else {
                        game->currentMove->firstPiecePlayersColor = false;
                        lightReport.reportId = 3;
                        lightReport.report3.reset = i << 3 | j;
                        USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&lightReport, 2);
                        USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceFS);
                        // TODO: DO WE WANT A DELAY??? Also, make sure lightsOn works properly by adding println debugging with segger_rtt
                    }
                    return;
                }
            } else if(!game->currentMove->isFinalState && game->currentMove->pickupState == FIRST_PIECE_PICKUP) { 

                // if a second piece is picked up, change state accordingly and update which piece was picked up
                if (!(i == clockModeReport.firstPickupRow && j == clockModeReport.firstPickupCol) && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                // TODO: ADD LIGHTS OFF AND THEN TURN ON LIGHT OF FIRST SQUARE AND PIECE PICKED UP SQUARE
                    // if (game->currentMove->lightsOn ) {
                    //     if(game->currentMove->allPieceLights[i][j] == 1) {
                    // update square info for picked up piece
                    clockModeReport.report2.secondPickupRow = i;
                    clockModeReport.report2.secondPickupCol = j;
                    clockModeReport.report2.finalPickupCol = 8;
                    clockModeReport.report2.finalPickupRow = 8;
                    game->currentMove->pickupState = SECOND_PIECE_PICKUP;
                    // if (game->currentMove->lightsOn && !game->currentMove->firstPiecePlayersColor && game->currentBoardState[i][j] == 0 && game->currentMove->allPieceLights[i][j] == 1 && game->previousState[i][j] == 1) {
                    if (game->currentMove->lightsOn && game->currentBoardState[i][j] == 0 && game->currentMove->allPieceLights[i][j] == 1 && game->previousState[i][j] == 1) {
                        memset(game->currentMove->lightState, 0, 64);
                        game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 1;
                        game->currentMove->lightState[i][j] = 1;
                        game->currentMove->lightsOn = true;
                        updateLights();
                    } 
                    /* } else {
                        // TODO: if piece picked up isn't valid second piece, blink lights or some shit. CHECK FOR EN PESSANT OR 
                    } */

                // if first piece picked up is put back on starting square, turn off lights and reset pickup state accordingly
                } else if (game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1) {
                    game->currentMove->pickupState = NO_PIECE_PICKUP;
                    lightsOff();
                    return;

                // if piece is moved over one of it's potential moves, only light up that square and it's original spot
                } else if (game->currentMove->lightsOn && game->currentMove->firstPiecePlayersColor && game->currentBoardState[i][j] == 1 && game->currentMove->allPieceLights[i][j] == 1 && game->previousState[i][j] == 0) {
                    game->currentMove->pieceNewSquare = true;
                    game->currentMove->pieceNewRow = i;
                    game->currentMove->pieceNewCol = j;
                    memset(game->currentMove->lightState, 0, 64);
                    game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 1;
                    game->currentMove->lightState[i][j] = 1;
                    game->currentMove->lightsOn = true;
                    updateLights();

                // first piece picked up was opponent's to take, and second piece is a valid one that is the player's piece
                // TODO: TEST THIS AND ABOVE IF STATEMENT AND MAKE SURE THAT THE LIGHTSON STATE IS ALWAYS CURRENT WITH PRINT STATEMENTS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                } else if (game->currentMove->pieceNewSquare && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol]) {
                    // TODO: timer debouncing shit for another animation if piece slides and then picked up again if we want????????????????????????????!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                }

            // if move is over by button press or timer
            } else if (!game->currentMove->isFinalState && game->currentMove->pickupState == SECOND_PIECE_PICKUP) {
                //TODO: MAKE IT SO EN PESSANT AND CASTLING WORK!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                //TODO: CHECK THAT WHERE PIECE IS SET DOWN IS VALID?????????????????????????????????????????!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! EXTRA CHECK NEEDED IF FIRST PICKUP WAS OPPONENT'S PIECE!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                
            } else if(game->currentMove->isFinalState) {
                bool enteredOne = false;
                // if it was a take, check to make sure piece was moved there
                if (game->currentMove->pickupState == SECOND_PIECE_PICKUP) {
                    game->currentMove->pieceNewSquare = false;

                    // TODO: ADD CHECK TO MAKE SURE BOTH SPOTS AREN'T 1, OR NOT NECESSARY???????????????????????????????????????????
                    // if spot where first or second piece was picked up is a 1, then that's the final spot the piece was moved and it's probably valid as long as error handling is added
                    if (game->currentBoardState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 1 || game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1) {
                        enteredOne = true;
                        
                    } else if (game->previousState[i][j] == 0 && game->currentBoardState[i][j] == 1) {
                        clockModeReport.report2.finalPickupRow = i;
                        clockModeReport.report2.finalPickupCol = j;
                        enteredOne = true;
                                               
                    }
                
                // as opposed to a take, if piece is moved, update report accordingly once final square is found
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
                    memcpy(game->previousState, game->currentBoardState, 8 * 8 * sizeof(game->previousState[0][0]));
                    memset(game->currentMove->allPieceLights, 0, 64);
                    memset(game->currentMove->lightState, 0, 64);
                    
                    return;
                } else {
                    // TODO: CHANGE THIS TO CHECK FOR IF IT DOESN"T ENTER SECOND PIECE PICKUP OR FIRST PIECE FINAL SPOT NOT FOUND, as now it'll just enter here whenever we're not on the final square
                }
            
            } 
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
        game.currentMove->lightsOn = true;
        uint8_t lights[8];
        convert2DArrayToBitarray(game.currentMove->allPieceLights, lights);       

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
    // TODO: FIX LIGHT BUG WHERE IF PIECE IMMEDIATELY SLID SUPER QUICK< WEIRD LIGHTS BEHAVIOR
    memset(game.currentMove->lightState, 0, 64);
    game.currentMove->lightsOn = true;

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
    game.currentMove->lightsOn = false;
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
