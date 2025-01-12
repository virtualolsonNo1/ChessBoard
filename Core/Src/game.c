#include "main.h"
#include "game.h"
#include "max7219.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"
#include "usb.h"
#include "usbd_def.h"
#include "usbd_customhid.h"
#include "cmsis_os2.h"
#include <ctype.h>


extern HIDClockModeReports clockModeReport;
extern USBD_HandleTypeDef hUsbDeviceFS;
extern SPI_HandleTypeDef hspi1;
extern struct GameState game;
extern osMutexId_t checkCastleSem;
extern struct ErrorMessage errorMessage;
extern bool isErrorState;
extern bool desktopError;
HIDClockModeReports lightReport;
bool isEnPassant = false;
bool moveIsCastling = false;
bool waitForCastlingResponse;
uint8_t lightsOffArr[8][8] = {0};
extern TIM_HandleTypeDef htim3;
bool startedPieceCheck = false;

// rotate 8 bit array around the center in case white is on other side of board
void rotate8x8Array(uint8_t rotateArr[8][8]) {
    uint8_t temp[8][8];
    memcpy(temp, rotateArr, 64);
    for(int i = 0; i < 8; i++) {
      for(int j = 0; j < 8; j++) {
        rotateArr[7 - i][7 - j] = temp[i][j];
      }
    }
}
void initTime(struct GameState* game) {
    //initialize the display with the starting time for both players
    max7219_PrintNtos(PLAYER1_MINUTES, game->player1->clock.minutes, 2);
    max7219_PrintNtos(PLAYER1_SECONDS, game->player1->clock.seconds, 2);
    max7219_PrintNtos(PLAYER2_MINUTES, game->player2->clock.minutes, 2);
    max7219_PrintNtos(PLAYER2_SECONDS, game->player2->clock.seconds, 2);
}

void displayNoClockBlack() {
    max7219_Decode_Off();
    // send blank for that digit
    max7219_SendData(DIGIT_1, 0x00);
    max7219_SendData(DIGIT_2, 0x00);
    max7219_SendData(DIGIT_3, 0x00);
    max7219_SendData(DIGIT_4, 0x00);
    max7219_SendData(DIGIT_5, 0x0E);
    max7219_SendData(DIGIT_7, 0x7E);
    
    max7219_SendData(DIGIT_8, 0x76);
    max7219_SendData(DIGIT_6, 0x4E);
}

void displayNoClockWhite() {
    max7219_Decode_Off();
    // send blank for that digit
    max7219_SendData(DIGIT_1, 0x0E);
    max7219_SendData(DIGIT_2, 0x4E);
    max7219_SendData(DIGIT_3, 0x7E);
    max7219_SendData(DIGIT_4, 0x76);
    max7219_SendData(DIGIT_5, 0x00);
    max7219_SendData(DIGIT_7, 0x00);
    
    max7219_SendData(DIGIT_8, 0x00);
    max7219_SendData(DIGIT_6, 0x00);
}

void displayNoClockBoth() {
    max7219_Decode_Off();
    // send blank for that digit
    max7219_SendData(DIGIT_1, 0x0E);
    max7219_SendData(DIGIT_2, 0x4E);
    max7219_SendData(DIGIT_3, 0x7E);
    max7219_SendData(DIGIT_4, 0x76);
    max7219_SendData(DIGIT_5, 0x0E);
    max7219_SendData(DIGIT_7, 0x7E);
    
    max7219_SendData(DIGIT_8, 0x76);
    max7219_SendData(DIGIT_6, 0x4E);
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
            game->timeControl = NO_CLOCK;
            break;
        case NO_CLOCK:
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
    if (game->timeControl == NO_CLOCK) {
        displayNoClockBoth();
    } else {
        max7219_Decode_On();
        initTime(game);
    }
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


bool checkCastling() {
    bool enteredOne = false;
    if ((game.previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'K' && game.previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 'R' ) || (game.previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'k' && game.previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 'r' )) {
        // if first piece picked up is a king, and to the left castling is possible and the rook is the one that was picked up, light up those squares 
        if (game.currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - 2] == 1 && clockModeReport.report2.secondPickupCol == 0) {
            memset(game.currentMove->lightState, 0, 64);
           game.currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - 1] = 1;  
           game.currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - 2] = 1;  
           enteredOne = true;
        } 
        if (game.currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + 2] == 1 && clockModeReport.report2.secondPickupCol == 7)  {
            memset(game.currentMove->lightState, 0, 64);
           game.currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + 1] = 1;  
           game.currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + 2] = 1;  
           enteredOne = true;
        } 
    
    } else if ((game.previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 'K' && game.previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'R' ) || (game.previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 'k' && game.previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'r' )) {
        // if first pickup is rook and second King, check for castling with desktop app!!!!!!!!!!!!!
        waitForCastlingResponse = true;
        lightReport.reportId = 3;
        lightReport.report3.reset = clockModeReport.report2.secondPickupRow << 3 | clockModeReport.report2.secondPickupCol;
        USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&lightReport, 2);
        osSemaphoreAcquire(checkCastleSem, osWaitForever);
        // USBD_CUSTOM_HID_ReceivePacket(&hUsbDeviceFS);
        waitForCastlingResponse = false;
        // if first piece picked up is a king, and to the left castling is possible and the rook is the one that was picked up, light up those squares 
        if (game.currentMove->allPieceLights[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol - 2] == 1 && clockModeReport.firstPickupCol == 0) {
            memset(game.currentMove->lightState, 0, 64);
           game.currentMove->lightState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol - 1] = 1;  
           game.currentMove->lightState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol - 2] = 1;  
           enteredOne = true;
        } 
        if (game.currentMove->allPieceLights[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol + 2] == 1 && clockModeReport.firstPickupCol == 7)  {
            memset(game.currentMove->lightState, 0, 64);
           game.currentMove->lightState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol + 1] = 1;  
           game.currentMove->lightState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol + 2] = 1;  
           enteredOne = true;
        } 
    }     

    // return true if castling is an option and the pieces picked up were 
    if (enteredOne) {
        moveIsCastling = true;
        return true;
    } else {
        return false;
    }
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
                    }
                    return;
                }
            } else if(!game->currentMove->isFinalState && game->currentMove->pickupState == FIRST_PIECE_PICKUP) { 

                // if a second piece is picked up, change state accordingly and update which piece was picked up
                if (!(i == clockModeReport.firstPickupRow && j == clockModeReport.firstPickupCol) && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                    // update square info for picked up piece
                    clockModeReport.report2.secondPickupRow = i;
                    clockModeReport.report2.secondPickupCol = j;
                    clockModeReport.report2.finalPickupCol = 8;
                    clockModeReport.report2.finalPickupRow = 8;
                    game->currentMove->pickupState = SECOND_PIECE_PICKUP;
                    
                    // TODO: I think this code is redundant, as won't enter final state otherwise, but keeping nonetheless
                    if (startedPieceCheck && game->timeControl == NO_CLOCK) {
                        // Stop the timer
                        HAL_TIM_Base_Stop(&htim3);
                        
                        // Reset the counter value to 10000
                        __HAL_TIM_SET_COUNTER(&htim3, 1000);
                        
                        // Clear any pending interrupt flag
                        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                        
                        startedPieceCheck = false;
                    }
                    
                    // if piece picked up is valid move as determined by the lights, handle accordingly
                    if (game->currentMove->lightsOn && game->currentMove->allPieceLights[i][j] == 1) {
                        memset(game->currentMove->lightState, 0, 64);
                        game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 1;
                        game->currentMove->lightState[i][j] = 1;
                        
                        // If en passant, light up final square for piece taking to land on as well as initial square for that piece
                        if ((game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'P' && game->previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 'p' && clockModeReport.firstPickupRow == clockModeReport.report2.secondPickupRow) || (game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'p' && game->previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 'P' && clockModeReport.firstPickupRow == clockModeReport.report2.secondPickupRow)) {
                            isEnPassant = true;
                            if (game->isWhiteMove) {
                                if (isupper(game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol])) {
                                    // en passant final square
                                    game->currentMove->lightState[clockModeReport.report2.secondPickupRow - 1][clockModeReport.report2.secondPickupCol] = 1;
                                    // keep track of en passant final square
                                    game->currentMove->pieceNewRow = clockModeReport.report2.secondPickupRow - 1;
                                    game->currentMove->pieceNewCol = clockModeReport.report2.secondPickupCol;
                                    
                                    // set square of 
                                    game->currentMove->lightState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] = 0;
                                } else {
                                    // en passant final square
                                    game->currentMove->lightState[clockModeReport.firstPickupRow - 1][clockModeReport.firstPickupCol] = 1;
                                    // keep track of en passant final square
                                    game->currentMove->pieceNewRow = clockModeReport.firstPickupRow - 1;
                                    game->currentMove->pieceNewCol = clockModeReport.firstPickupCol;

                                    game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 0;
                                }
                            } else {
                                if (islower(game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol])) {
                                    // en passant final square
                                    game->currentMove->lightState[clockModeReport.report2.secondPickupRow + 1][clockModeReport.report2.secondPickupCol] = 1;
                                    // keep track of en passant final square
                                    game->currentMove->pieceNewRow = clockModeReport.report2.secondPickupRow + 1;
                                    game->currentMove->pieceNewCol = clockModeReport.report2.secondPickupCol;

                                    game->currentMove->lightState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] = 0;
                                } else {
                                    // en passant final square
                                    game->currentMove->lightState[clockModeReport.firstPickupRow + 1][clockModeReport.firstPickupCol] = 1;
                                    // keep track of en passant final square
                                    game->currentMove->pieceNewRow = clockModeReport.firstPickupRow + 1;
                                    game->currentMove->pieceNewCol = clockModeReport.firstPickupCol;

                                    game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 0;
                                }
                            }
                        }

                        game->currentMove->lightsOn = true;
                        updateLights();
                        
                        
                    } else {
                        // not potential final spot for piece, but could be en passant or castling
                        if (checkCastling()) {
                            moveIsCastling = true;
                            game->currentMove->lightsOn = true;
                            updateLights();

                        // if second move is not possible given first piece picked up, blink error!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        } else {
                            // set isErrorState to true so update move thread can suspend itself later and start blink error task
                            isErrorState = true;
                            errorMessage.numPieces = 1;
                            errorMessage.resetState = FIRST_PIECE_PICKUP;
                            
                            // send in firstPickupRow and firstPickupCol so it knows what initial piece was picked up and is still picked up
                            errorMessage.firstPickupRow = clockModeReport.firstPickupRow;
                            errorMessage.firstPickupCol = clockModeReport.firstPickupCol;
                        }
                    }

                // if first piece picked up is put back on starting square, turn off lights and reset pickup state accordingly
                } else if (game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1) {
                    startedPieceCheck = false;
                    game->currentMove->pickupState = NO_PIECE_PICKUP;
                    game->currentMove->receivedLightData = false;
                    game->currentMove->lightsOn = false;
                    game->currentMove->pieceNewSquare = false;
                    lightsOff();
                    return;

                // if piece is moved over one of it's potential moves, only light up that square and it's original spot
                } else if (game->currentMove->lightsOn && game->currentMove->firstPiecePlayersColor && game->currentBoardState[i][j] == 1 && game->currentMove->allPieceLights[i][j] == 1 && game->previousState[i][j] == 0) {
                    if (!startedPieceCheck && game->timeControl == NO_CLOCK && !((game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'K' || game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'k') && clockModeReport.firstPickupCol == 4 && ((game->currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + 2] == 1 && clockModeReport.firstPickupRow == i && clockModeReport.firstPickupCol + 2 == j) || (game->currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - 2] == 1 && clockModeReport.firstPickupRow == i && clockModeReport.firstPickupCol - 2 == j)))) {

                        // Clear any pending interrupt flag
                        NVIC_DisableIRQ(TIM3_IRQn);
                        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                        
                        // Enable the update interrupt
                        htim3.Instance->DIER |= TIM_DIER_UIE;
                        
                        __HAL_TIM_SET_COUNTER(&htim3, 1000);
                        // Start the timer
                        HAL_TIM_Base_Start(&htim3);
                        NVIC_EnableIRQ(TIM3_IRQn);

                        startedPieceCheck = true;
                        volatile int x = 1;
                    } else if (startedPieceCheck && game->timeControl == NO_CLOCK && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol] == 0) {
                    // Stop the timer
                        HAL_TIM_Base_Stop(&htim3);
                        
                        // Reset the counter value to 10000
                        __HAL_TIM_SET_COUNTER(&htim3, 1000);
                        
                        // Clear any pending interrupt flag
                        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                        
                        startedPieceCheck = false;
                    }
                    game->currentMove->pieceNewSquare = true;
                    game->currentMove->pieceNewRow = i;
                    game->currentMove->pieceNewCol = j;
                    memset(game->currentMove->lightState, 0, 64);
                    game->currentMove->lightState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] = 1;
                    game->currentMove->lightState[i][j] = 1;
                    game->currentMove->lightsOn = true;
                    updateLights();

                // first piece picked up was opponent's to take, and second piece is a valid one that is the player's piece
                } else if (game->currentMove->pieceNewSquare && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol]) {
                    // TODO: timer debouncing shit for another animation if piece slides and then picked up again if we want????????????????????????????!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                }

            // if second piece picked up
            } else if (!game->currentMove->isFinalState && game->currentMove->pickupState == SECOND_PIECE_PICKUP) {

                // if previous state is equal to current board state, go back to no piece pickup state
                if (memcmp(game->previousState, game->currentBoardState, 64) == 0) {
                    isEnPassant = false;
                    moveIsCastling = false;
                    // game->currentMove->pieceNewSquare = false;

                    game->currentMove->pickupState = NO_PIECE_PICKUP;
                    lightsOff();
                    
                // if a third piece is picked up that was previously down, go to error state till it's put down
                } else if (!(i == clockModeReport.firstPickupRow && j == clockModeReport.firstPickupCol) && !(i == clockModeReport.report2.secondPickupRow && j == clockModeReport.report2.secondPickupCol) && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                    isErrorState = true;
                    errorMessage.numPieces = 2;
                    errorMessage.resetState = SECOND_PIECE_PICKUP;
                    return;
                } 
                
                // if check for piece on final square not started and second piece picked up is final square, start check for said square
                if (!isEnPassant && !startedPieceCheck && game->timeControl == NO_CLOCK && game->currentBoardState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 1 && ((!game->isWhiteMove && isupper(game->previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol])) || (game->isWhiteMove && islower(game->previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol])))) {
                    game->currentMove->pieceNewRow = clockModeReport.report2.secondPickupRow;
                    game->currentMove->pieceNewCol = clockModeReport.report2.secondPickupCol;
                    NVIC_DisableIRQ(TIM3_IRQn);
                    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                    
                    // Enable the update interrupt
                    htim3.Instance->DIER |= TIM_DIER_UIE;
                    
                    __HAL_TIM_SET_COUNTER(&htim3, 1000);
                    // Start the timer
                    HAL_TIM_Base_Start(&htim3);
                    NVIC_EnableIRQ(TIM3_IRQn);

                    startedPieceCheck = true;
                    volatile int x = 1;
                    
                // if check for piece on final square not started and first piece picked up is final square, start check for said square
                } else if (!isEnPassant && !startedPieceCheck && game->timeControl == NO_CLOCK && game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1 && ((!game->isWhiteMove && isupper(game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol])) || (game->isWhiteMove && islower(game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol])))) {
                    // game->currentMove->pieceNewSquare = true;
                    game->currentMove->pieceNewRow = clockModeReport.firstPickupRow;
                    game->currentMove->pieceNewCol = clockModeReport.firstPickupCol;
                    NVIC_DisableIRQ(TIM3_IRQn);
                    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                    
                    // Enable the update interrupt
                    htim3.Instance->DIER |= TIM_DIER_UIE;
                    
                    __HAL_TIM_SET_COUNTER(&htim3, 1000);
                    // Start the timer
                    HAL_TIM_Base_Start(&htim3);
                    NVIC_EnableIRQ(TIM3_IRQn);

                    startedPieceCheck = true;
                    volatile int x = 1;
                    
                // if check for piece on final square not started and castling is occurring, start check for final castling squares to see if both have a piece on them
                } else if (!isEnPassant && !moveIsCastling && startedPieceCheck && game->timeControl == NO_CLOCK && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol] == 0) {
                    // Stop the timer
                    HAL_TIM_Base_Stop(&htim3);
                    // Reset the counter value to 10000
                    __HAL_TIM_SET_COUNTER(&htim3, 1000);
                    // Clear any pending interrupt flag
                    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                    startedPieceCheck = false;
                } else if (moveIsCastling && !startedPieceCheck && game->timeControl == NO_CLOCK) {
                    int numPiecesOnLights = 0;
                    int firstLightRow = 8;
                    int firstLightCol = 8;
                    int secondLightRow = 8;
                    int secondLightCol = 8;
                    for(int a = 0; a < 8; a++) {
                        for(int b = 0; b < 8; b++) {
                            if (game->currentMove->allPieceLights[a][b] == 1 && game->currentBoardState[a][b] == 1) {
                                numPiecesOnLights++;
                                if (numPiecesOnLights == 1) {
                                    firstLightRow = a;
                                    firstLightCol = b;
                                } else if (numPiecesOnLights == 2) {
                                    secondLightRow = a;
                                    secondLightCol = b;
                                }
                            }
                        }
                    }
                    
                    // if there are pieces on both castling final squares, start timer for checking they're there for 1 second
                    if (numPiecesOnLights == 2) {
                        // game->currentMove->pieceNewSquare = true;
                        game->currentMove->pieceNewRow = firstLightRow;
                        game->currentMove->pieceNewCol = firstLightCol;
                        game->currentMove->secondPieceNewRow = secondLightRow;
                        game->currentMove->secondPieceNewCol = secondLightCol;
                        NVIC_DisableIRQ(TIM3_IRQn);
                        __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                        // Enable the update interrupt
                        htim3.Instance->DIER |= TIM_DIER_UIE;
                        __HAL_TIM_SET_COUNTER(&htim3, 1000);
                        // Start the timer
                        HAL_TIM_Base_Start(&htim3);
                        NVIC_EnableIRQ(TIM3_IRQn);
                        startedPieceCheck = true;
                        volatile int x = 1;
                    }
                } else if (moveIsCastling && startedPieceCheck && game->timeControl == NO_CLOCK && (game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol] == 0 || game->currentBoardState[game->currentMove->secondPieceNewRow][game->currentMove->secondPieceNewCol] == 0)) {
                    // Stop the timer
                    HAL_TIM_Base_Stop(&htim3);
                    // Reset the counter value to 10000
                    __HAL_TIM_SET_COUNTER(&htim3, 1000);
                    // Clear any pending interrupt flag
                    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                    startedPieceCheck = false;
                    
                // if en passant is move played by two pieces that were picked up, and a piece is on the final square for said move, start the timer
                } else if (isEnPassant && !startedPieceCheck && game->timeControl  == NO_CLOCK && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol] == 1) {
                    // game->currentMove->pieceNewSquare = true;
                    NVIC_DisableIRQ(TIM3_IRQn);
                    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                    // Enable the update interrupt
                    htim3.Instance->DIER |= TIM_DIER_UIE;
                    __HAL_TIM_SET_COUNTER(&htim3, 1000);
                    // Start the timer
                    HAL_TIM_Base_Start(&htim3);
                    NVIC_EnableIRQ(TIM3_IRQn);
                    startedPieceCheck = true;
                
                // if en passant is move played by two pieces that were picked up, and timer was started due to piece on final square, but the piece is no longer there, stop the timer
                } else if (isEnPassant && startedPieceCheck && game->timeControl  == NO_CLOCK && game->currentBoardState[game->currentMove->pieceNewRow][game->currentMove->pieceNewCol] == 0) {
                    // Stop the timer
                    HAL_TIM_Base_Stop(&htim3);
                    // Reset the counter value to 10000
                    __HAL_TIM_SET_COUNTER(&htim3, 1000);
                    // Clear any pending interrupt flag
                    __HAL_TIM_CLEAR_FLAG(&htim3, TIM_FLAG_UPDATE);
                    startedPieceCheck = false;
                }


            // if final state, check that valid move occurred and move to next 
            } else if(game->currentMove->isFinalState) {
                bool enteredOne = false;
                // if it was a take, check to make sure piece was moved there
                if (game->currentMove->pickupState == SECOND_PIECE_PICKUP) {
                    // game->currentMove->pieceNewSquare = false;

                    // if spot where first or second piece was picked up is a 1, then that's the final spot the piece was moved and it's probably valid as long as error handling is added
                    if (!moveIsCastling && (game->currentBoardState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 1 || game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1 || isEnPassant)) {
                        if (isEnPassant) {
                            bool entered = false;
                            // isEnPassant = false;
                            for(int a = 0; a < 8; a++) {
                                for(int b = 0; b < 8; b++) {
                                    // if there's a piece on a square where the lights are lit up, that's final spot for piece
                                    if (game->currentMove->lightState[a][b] == 1 && game->currentBoardState[a][b] == 1) {
                                        entered = true;   
                                        clockModeReport.report2.finalPickupRow = a;
                                        clockModeReport.report2.finalPickupCol = b;
                                    }
                                }
                            }

                            // if piece isn't on one of it's valid spots, go to error state
                            if (!entered) {
                                isErrorState = true;
                                errorMessage.numPieces = 1;
                                desktopError = true;
                                errorMessage.resetState = NO_PIECE_PICKUP;
                                return;
                            }
                        }

                        // if the piece is put back on it's starting square instead of new square (i.e. the piece where it landed matches who's move it is), error handle
                        if (game->currentBoardState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 1 && ((game->isWhiteMove && isupper(game->previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol])) || (!game->isWhiteMove && islower(game->previousStateChar[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol])))) {
                            isErrorState = true;
                            errorMessage.numPieces = 1;
                            desktopError = true;
                            errorMessage.resetState = NO_PIECE_PICKUP;
                            return;

                        } else if (game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 1 && ((game->isWhiteMove && isupper(game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol])) || (!game->isWhiteMove && islower(game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol])))) {
                            isErrorState = true;
                            errorMessage.numPieces = 1;
                            desktopError = true;
                            errorMessage.resetState = NO_PIECE_PICKUP;
                            return;

                        }

                        enteredOne = true;
                        
                    } else if (!moveIsCastling && (game->previousState[i][j] == 0 && game->currentBoardState[i][j] == 1 && game->currentMove->lightState[i][j] != 1)) {
                        isErrorState = true;
                        errorMessage.numPieces = 1;
                        desktopError = true;
                        errorMessage.resetState = NO_PIECE_PICKUP;
                        return;
                    } else if (moveIsCastling) {
                        uint8_t numDifferences = 0;
                        for(int i = 0; i < 8; i++) {
                            for(int j = 0; j < 8; j++) {
                                if (game->previousState[i][j] == 0 && game->currentBoardState[i][j] == 1 && game->currentMove->lightState[i][j] == 1) {
                                    numDifferences++;
                                }
                            }
                        }
                        
                        // if pieces are on the two lit up, valid squares, and  nothing else is different, should be chilling
                        if (numDifferences == 2) {
                            enteredOne = true;

                        // if more than two differences, enter error state accordingly
                        } else if (!isErrorState) {
                        uint8_t errorStatus;
                        memcpy(&errorStatus, hUsbDeviceFS.pClassData + 1, sizeof(1));
                        isErrorState = true;
                        errorMessage.numPieces = 1;
                        desktopError = true;
                        errorMessage.resetState = NO_PIECE_PICKUP;
                        return;
                        }

                    // if it's not en passant or castling and nothing is on the first and second pickup squares, need to error out
                    } else if (!isErrorState && !isEnPassant && !moveIsCastling && game->currentBoardState[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 0 && game->currentBoardState[clockModeReport.report2.secondPickupRow][clockModeReport.report2.secondPickupCol] == 0) {
                        uint8_t errorStatus;
                        memcpy(&errorStatus, hUsbDeviceFS.pClassData + 1, sizeof(1));
                        isErrorState = true;
                        errorMessage.numPieces = 1;
                        desktopError = true;
                        errorMessage.resetState = NO_PIECE_PICKUP;
                        return;
                    }
                
                // as opposed to a take, if piece is moved, update report accordingly once final square is found
                } else {
                    bool onNewSquare = false;
                    for(int a = 0; a < 8; a++) {
                        for(int b = 0; b < 8; b++) {
                            // if piece is on new square and that square is lit up, probably valid spot besides one king edge case
                            if (game->previousState[a][b] == 0 && game->currentBoardState[a][b] == 1 && game->currentMove->lightState[a][b] == 1) {
                                onNewSquare = true;
                                    // if king is moved two spots from current one, that shit aint legal 
                                    if ((game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'K' || game->previousStateChar[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol] == 'k') && clockModeReport.firstPickupCol == 4 && ((game->currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + 2] == 1 && clockModeReport.firstPickupRow == i && clockModeReport.firstPickupCol + 2 == j) || (game->currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - 2] == 1 && clockModeReport.firstPickupRow == i && clockModeReport.firstPickupCol - 2 == j))) {
                                        isErrorState = true;
                                        errorMessage.numPieces = 1;
                                        desktopError = true;
                                        errorMessage.resetState = NO_PIECE_PICKUP;
                                        return;
                                    }
                                    clockModeReport.report1.finalPickupRow = a;
                                    clockModeReport.report1.finalPickupCol = b;
                                    enteredOne = true;
                            }
                        }
                    }
                    
                    // if piece not on new square, error occurred
                    if (!onNewSquare) {
                        isErrorState = true;
                        errorMessage.numPieces = 1;
                        desktopError = true;
                        errorMessage.resetState = NO_PIECE_PICKUP;
                        return;
                        
                    }
                }                      
                if (enteredOne) {
                    int numDifferences = 0;
                    for (int a = 0; a < 8; a++) {
                        for(int b = 0; b < 8; b++) {
                            if (game->currentBoardState[a][b] != game->previousState[a][b]) {
                                numDifferences++;
                            }
                        }
                    }

                    // make sure num differences matches up with the amount it should be given kind of move played, otherwise error out
                    if (!((numDifferences == 3 && isEnPassant) || (numDifferences == 4 && moveIsCastling) || (game->currentMove->pickupState == FIRST_PIECE_PICKUP && numDifferences == 2) || (game->currentMove->pickupState == SECOND_PIECE_PICKUP && numDifferences == 1))) {
                        isErrorState = true;
                        errorMessage.numPieces = 1;
                        desktopError = true;
                        errorMessage.resetState = NO_PIECE_PICKUP;
                        return;
                        
                    }

                    return;
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
        if (game.gameStarted && !game.player1IsWhite)
            rotate8x8Array(game.currentMove->allPieceLights);     
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
      
      
        if (game.gameStarted && !game.player1IsWhite)
            rotate8x8Array(game.currentMove->allPieceLights);     
}

void updateLights() {
    if (game.gameStarted && !game.player1IsWhite)
        rotate8x8Array(game.currentMove->lightState);     

    uint8_t eightBitLights[8];
    convert2DArrayToBitarray(game.currentMove->lightState, eightBitLights);       

    volatile int test = HAL_SPI_Transmit(&hspi1, (uint8_t *)eightBitLights, 8, 10000);
    while(!(SPI1->SR & 0b10)) {}

    //after transmitting LED data to shift registers, assert and de-assert load pin to display those values
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
    while(!(GPIOA->ODR & GPIO_PIN_10)) {}
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
    while((GPIOA->ODR & GPIO_PIN_10)) {}

    if (game.gameStarted && !game.player1IsWhite)
        rotate8x8Array(game.currentMove->lightState);     

    osDelay(5);
}


void animateInitialLights() {
    uint8_t tempLights[8][8] = {0};
    game.currentMove->lightsOn = true;

    int rowMax = clockModeReport.firstPickupRow < 4 ? 7 - clockModeReport.firstPickupRow : clockModeReport.firstPickupRow;
    int colMax = clockModeReport.firstPickupCol < 4 ? 7 - clockModeReport.firstPickupCol : clockModeReport.firstPickupCol;
    int maxVal = (rowMax >= colMax) ? rowMax : colMax;
    for(int i = 1; i < 8; i++) {
        uint8_t eightBitLights[8] = {0};
        // check below to see if light should go on
        if (clockModeReport.firstPickupRow + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol] == 1) {
            tempLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol] = 1;
        }
        // check below and to the right if light should go on
        if (clockModeReport.firstPickupRow + i <= 7 && clockModeReport.firstPickupCol + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol + i] == 1) {
            tempLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol + i] = 1;
        }
        // check below and to the left if light should go on
        if (clockModeReport.firstPickupRow + i <= 7 && clockModeReport.firstPickupCol - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol - i] == 1) {
            tempLights[clockModeReport.firstPickupRow + i][clockModeReport.firstPickupCol - i] = 1;
        }

        // check above to see if lights should go on
        if (clockModeReport.firstPickupRow - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol] == 1) {
            tempLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol] = 1;
        }
        // check above and to the right if light should go on
        if (clockModeReport.firstPickupRow - i >= 0 && clockModeReport.firstPickupCol + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol + i] == 1) {
            tempLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol + i] = 1;
        }
        // check above and to the left if light should go on
        if (clockModeReport.firstPickupRow - i >= 0 && clockModeReport.firstPickupCol - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol - i] == 1) {
            tempLights[clockModeReport.firstPickupRow - i][clockModeReport.firstPickupCol - i] = 1;
        }

        // check to the right to see if lights should go on
        if (clockModeReport.firstPickupCol + i <= 7 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + i] == 1) {
            tempLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol + i] = 1;
        }
        // check to the left to see if lights should go on
        if (clockModeReport.firstPickupCol - i >= 0 && game.currentMove->allPieceLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - i] == 1) {
            tempLights[clockModeReport.firstPickupRow][clockModeReport.firstPickupCol - i] = 1;
        }
        // update lights until piece placed back on OG square, second piece picked up, or first piece moved over new (valid) square
        if (!((game.currentMove->pieceNewSquare && game.currentMove->pickupState == FIRST_PIECE_PICKUP) || game.currentMove->pickupState == SECOND_PIECE_PICKUP || game.currentMove->pickupState == NO_PIECE_PICKUP)) {
            if (game.gameStarted && !game.player1IsWhite)
                rotate8x8Array(tempLights);     

            uint8_t eightBitLights[8];
            convert2DArrayToBitarray(tempLights, eightBitLights);       

            volatile int test = HAL_SPI_Transmit(&hspi1, (uint8_t *)eightBitLights, 8, 10000);
            while(!(SPI1->SR & 0b10)) {}

            //after transmitting LED data to shift registers, assert and de-assert load pin to display those values
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);
            while(!(GPIOA->ODR & GPIO_PIN_10)) {}
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
            while((GPIOA->ODR & GPIO_PIN_10)) {}

            osDelay(5);
        } else {
            // if (game.currentMove->pickupState == NO_PIECE_PICKUP)
            // lightsOff();
            return;
        }

        osDelay(100);
    }
    osDelay(5);
}


void lightsOff() {
    game.currentMove->receivedLightData = false;
    game.currentMove->lightsOn = false;
    memset(game.currentMove->lightState, 0, 64);
    memset(game.currentMove->allPieceLights, 0, 64);
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
