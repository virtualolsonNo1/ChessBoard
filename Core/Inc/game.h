#include "stdint.h"
#include "stdbool.h"
#include "stm32f4xx_hal.h"
// #include "stm32f411xe.h"
// #include "stm32f4xx_hal_tim.h"

#define PLAYER1_MINUTES     DIGIT_4
#define PLAYER1_SECONDS     DIGIT_2
#define PLAYER2_MINUTES     DIGIT_8
#define PLAYER2_SECONDS     DIGIT_6

#define ONE_MIN             1
#define TWO_MIN             2
#define THREE_MIN           3
#define FIVE_MIN            5
#define TEN_MIN             10
#define THIRTY_MIN          30
#define HOUR                60

typedef enum {
    ONE_MINUTE_LIMIT = 60000,
    TWO_MINUTE_LIMIT = 120000,
    THREE_MINUTE_LIMIT = 180000,
    FIVE_MINUTE_LIMIT = 300000,
    TEN_MINUTE_LIMIT = 600000,
    THIRTY_MINUTE_LIMIT = 1800000,
    HOUR_LIMIT = 3600000,
} AllowedTimes;

struct Clock {
    TIM_HandleTypeDef* timer;
    uint8_t minutes;
    uint8_t seconds;
};

struct Player {
    struct Clock clock;
    bool isWhite;
};

struct GameState {
    struct Player* activePlayer;
    struct Player* player1;
    struct Player* player2;
    bool gameStarted;
    AllowedTimes timeControl;
    bool resetNow;
};

void initTime(struct GameState* game);
void changeTimeControl(struct GameState* game);
// void updateTime(struct GameState* game);
void resetGame(struct GameState* game);