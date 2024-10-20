#include "stdint.h"
#include "stdbool.h"
#include "stm32f4xx_hal.h"

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

enum PickupState {
    NO_PIECE_PICKUP,
    FIRST_PIECE_PICKUP,
    SECOND_PIECE_PICKUP,
    FINAL_STATE
};

struct MoveState {
    bool lightsOn;
    bool receivedLightData;
    enum PickupState pickupState;
    bool firstPiecePlayersColor;
    bool isFinalState;
    uint8_t allPieceLights[8][8];
    uint8_t lightState[8][8];
    bool pieceNewSquare;
    uint8_t pieceNewRow;
    uint8_t pieceNewCol;
};


struct GameState {
    struct Player* activePlayer;
    struct Player* player1;
    struct Player* player2;
    bool gameStarted;
    AllowedTimes timeControl;
    bool resetNow;
    bool isWhiteMove;
    struct MoveState* currentMove;
    uint8_t previousState[8][8];
    unsigned char previousStateChar[8][8];
    //TODO: PROBABLY WANT TO ADD BACK FOR NO CLOCK MODE!!!
    // char chessBoard[8][8];
    uint8_t currentBoardState[8][8];
};

void initTime(struct GameState* game);
void changeTimeControl(struct GameState* game);
void updateMoveShit(struct GameState* game);
void resetGame(struct GameState* game);
void updateReceivedLights();
void updateLights();
void animateInitialLights();
void lightsOff();
void checkStartingSquares();