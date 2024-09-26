#include <stdint.h>
#define HID_REPORT1_SIZE 67
#define HID_REPORT2_SIZE 69

typedef struct __attribute__((packed)) {
    uint8_t reportId;
    uint8_t firstPickupCol;
    uint8_t firstPickupRow;
    uint8_t secondPickupState[8][8];
} HID_Report1_t;

typedef struct __attribute__((packed)) {
    uint8_t reportId;
    uint8_t firstPickupCol;
    uint8_t firstPickupRow;
    uint8_t secondPickupCol;
    uint8_t secondPickupRow;
    uint8_t thirdPickupState[8][8];
} HID_Report2_t;

// Union of both report types
typedef union {
    HID_Report1_t report1;
    HID_Report2_t report2;
} HID_Report_u;