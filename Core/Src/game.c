#include "main.h"
#include "game.h"
#include "max7219.h"
#include "stm32f4xx_hal_tim.h"

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

// void updateTime(struct GameState* game) {
//     //grab count value from CNT register of the active player's timer
//     int count = game->activePlayer->clock.timer->Instance->CNT;

//     //convert count to minutes and seconds
//     volatile int totalSeconds = count / 25000;
//     minutes = totalSeconds / 60;
//     secondsRemaining = totalSeconds - (minutes * 60);

//     uint8_t minutesReg;
//     uint8_t secondsReg;

//     //use correct register values for display depending on who is the active player
//     if(game->activePlayer == game->player1) {
//             minutesReg = PLAYER1_MINUTES;
//             secondsReg = PLAYER1_SECONDS;
//         } else {
//             minutesReg = PLAYER2_MINUTES;
//             secondsReg = PLAYER2_SECONDS;
//         }

//     //if the current player's clock has different time than is displayed, update the display accordingly
//     if(secondsRemaining != game->activePlayer->clock.seconds && minutes != game->activePlayer->clock.minutes) {
//         game->activePlayer->clock.seconds = secondsRemaining;
//         game->activePlayer->clock.minutes = minutes;
//         max7219_PrintNtos(minutesReg, minutes, 2);
//         max7219_PrintNtos(secondsReg, secondsRemaining, 2);
//     } else if (secondsRemaining != game->activePlayer->clock.seconds) {
//         max7219_PrintNtos(secondsReg, secondsRemaining, 2);
//     }
//     if (game->resetNow) {
//         game->resetNow = false;
//         initTime(game);
//     }
// }

void resetGame(struct GameState* game) {
    //TODO: reset game to previous time control
    //reset state of game's different fields
    game->timeControl = ONE_MINUTE_LIMIT;
    game->player1->clock.minutes = 1;
    game->player1->clock.seconds = 0;
    game->player2->clock.minutes = 1;
    game->player2->clock.seconds = 0;
    game->gameStarted = false;
    game->resetNow = true;

    //reset ARR to correct value
    __HAL_TIM_SET_AUTORELOAD(game->player1->clock.timer, game->timeControl);
    __HAL_TIM_SET_AUTORELOAD(game->player2->clock.timer, game->timeControl);
    HAL_TIM_Base_Init(game->player1->clock.timer);
    HAL_TIM_Base_Init(game->player2->clock.timer);
    minutes = 1;
    secondsRemaining = 0;
    return;
}