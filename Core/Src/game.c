#include "main.h"
#include "game.h"
#include "max7219.h"
#include "stm32f4xx_hal_tim.h"
#include "string.h"

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

    //TODO: might wanna add back later for error checking!!!
    // char newGame[8][8] = {
    //     {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
    //     {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
    //     {0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0},
    //     {0, 0, 0, 0, 0, 0, 0, 0},
    //     {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
    //     {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
    // };
    // memcpy(&game->chessBoard, &newGame, 8 * 8 * sizeof(char));

    minutes = 1;
    secondsRemaining = 0;

    //reset ARR to correct value
    __HAL_TIM_SET_AUTORELOAD(game->player1->clock.timer, game->timeControl);
    __HAL_TIM_SET_AUTORELOAD(game->player2->clock.timer, game->timeControl);
    HAL_TIM_Base_Init(game->player1->clock.timer);
    HAL_TIM_Base_Init(game->player2->clock.timer);
    return;
}

void updateMoveShit(struct GameState* game) {
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(!game->currentMove->firstPiecePickup && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                //piece was picked up!!!
                uint8_t temp [8][8];
                memcpy(&temp, &game->previousState, 8 * 8 * sizeof(game->previousState[0][0]));
                temp[i][j] = 0;
                memcpy(&game->currentMove->firstPickupState, &temp, 8 * 8 * sizeof(temp[0][0]));
                game->currentMove->firstPiecePickup = true;
                //TODO: test if this code is fixed for picking up a piece, moving it over a square, then moving it to a different square
            } else if(game->currentMove->firstPiecePickup && !game->currentMove->secondPiecePickup && !game->currentMove->isFinalState && game->currentMove->firstPickupState[i][j] == 1 && game->previousState[i][j] == 1 && game->currentBoardState[i][j] == 0) {
                //piece is removed to be taken, save state!!!!!!
                uint8_t temp [8][8];
                memcpy(&temp, &game->previousState, 8 * 8 * sizeof(game->previousState[0][0]));
                temp[i][j] = 0;
                memcpy(&game->currentMove->secondPickupState, &temp, 8 * 8 * sizeof(temp[0][0]));
                game->currentMove->secondPiecePickup = true;
            } else if(game->currentMove->isFinalState) {
                //button was hit to finish this move and change to other player's move, so save final state and update previous state to current state
                memcpy(game->currentMove->finalState, game->currentBoardState, 8 * 8 * sizeof(game->currentMove->finalState[0][0]));
                memcpy(game->previousState, game->currentBoardState, 8 * 8 * sizeof(game->previousState[0][0]));
            }


        }
    }
}