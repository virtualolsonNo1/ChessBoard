#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t reportId;
    uint8_t firstPickupCol;
    uint8_t firstPickupRow;
    uint8_t secondPickupState[8][8];
} report1;

typedef struct __attribute__((packed)) {
    uint8_t reportId;
    uint8_t firstPickupCol;
    uint8_t firstPickupRow;
    uint8_t secondPickupColumn;
    uint8_t secondPickupRow;
    uint8_t thirdPickupState[8][8];
} report2;