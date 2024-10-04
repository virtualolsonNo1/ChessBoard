#include "main.h"
#include "game.h"
#include "max7219.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"
#include "usb.h"
#include "usbd_def.h"
#include "usbd_customhid.h"

extern HIDClockModeReports clockModeReport;
extern USBD_HandleTypeDef hUsbDeviceFS;

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

      memcpy(game->previousState, previousState, 8 * 8 * sizeof(previousState[0][0]));

    //reset ARR to correct value
    __HAL_TIM_SET_AUTORELOAD(game->player1->clock.timer, game->timeControl);
    __HAL_TIM_SET_AUTORELOAD(game->player2->clock.timer, game->timeControl);
    HAL_TIM_Base_Init(game->player1->clock.timer);
    HAL_TIM_Base_Init(game->player2->clock.timer);
    
    // send reset game report to desktop app
    clockModeReport.reportId = 3;
    clockModeReport.report3.reset = 1;
    USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 2);
    // HAL_Delay(50);
    
    return;
}

void updateMoveShit(struct GameState* game) {
    if(game->gameStarted) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(!game->currentMove->firstPiecePickup && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                //first piece was picked up!!!
                // uint8_t temp [8][8];
                // memcpy(&temp, &game->previousState, 8 * 8 * sizeof(game->previousState[0][0]));
                // temp[i][j] = 0;
                // memcpy(&game->currentMove->firstPickupState, &temp, 8 * 8 * sizeof(temp[0][0]));
                clockModeReport.firstPickupRow = i;
                clockModeReport.firstPickupCol = j;
                game->currentMove->firstPiecePickup = true;
                //TODO: test if this code is fixed for picking up a piece, moving it over a square, then moving it to a different square
            } else if(game->currentMove->firstPiecePickup && !game->currentMove->secondPiecePickup && !game->currentMove->isFinalState && !(i == clockModeReport.firstPickupRow && j == clockModeReport.firstPickupCol) && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                clockModeReport.report2.secondPickupRow = i;
                clockModeReport.report2.secondPickupCol = j;
                // second piece was picked up
                // uint8_t temp [8][8];
                // memcpy(&temp, &game->currentMove->firstPickupState, 8 * 8 * sizeof(game->currentMove->firstPickupState[0][0]));
                // temp[i][j] = 0;
                // memcpy(&game->currentMove->secondPickupState, &temp, 8 * 8 * sizeof(temp[0][0]));
                game->currentMove->secondPiecePickup = true;
            } else if(game->currentMove->isFinalState) {
                for(int i = 0; i < 8; i++) {
                    for(int j = 0; j < 8; j++) {
                        if (game->previousState[i][j] == 0 && game->currentBoardState[i][j] == 1) {
                            clockModeReport.report2.finalPickupRow = i;
                            clockModeReport.report2.finalPickupCol = j;
                        }
                    }
                }
                //button was hit to finish this move and change to other player's move, so save final state and update previous state to current state
                if (!game->currentMove->secondPiecePickup) {
                // memcpy(clockModeReport.secondPickupState, game->currentBoardState, 8 * 8 * sizeof(game->previousState[0][0]));
                } else {
                // memcpy(clockModeReport.thirdPickupState, game->currentBoardState, 8 * 8 * sizeof(game->previousState[0][0]));
                }
                memcpy(game->previousState, game->currentBoardState, 8 * 8 * sizeof(game->previousState[0][0]));
            }


        }
    }
    }
}