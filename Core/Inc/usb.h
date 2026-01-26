#include <stdint.h>

// USB RELATED MACROS
// Report ID macros
#define PIECE_MOVED_REPORT_ID 1
#define PIECE_TAKEN_OR_CASTLING_REPORT_ID 2
#define RESET_OR_LIGHT_REQUEST_REPORT_ID 3
#define FRONTEND_DATA_REPORT_ID 7
#define FRONTEND_DATA_ERROR_REPORT_ID 8

// Frontend reason macros
#define NO_PIECE_PICKUP_FRONTEND_REASON 1
#define PIECE_MOVING_FRONTEND_REASON 2
#define SECOND_PIECE_PICKED_UP_FRONTEND_REASON 3

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
typedef struct __attribute__((packed)) {
    uint8_t reportId;
    union {
        struct __attribute__((packed)) {
            uint8_t firstPickupRow;
            uint8_t firstPickupCol;
            union {
                struct __attribute__((packed)) {
                    uint8_t finalPickupRow;
                    uint8_t finalPickupCol;
                } report1;
                struct __attribute__((packed)) {
                    uint8_t secondPickupRow;
                    uint8_t secondPickupCol;
                    uint8_t finalPickupRow;
                    uint8_t finalPickupCol;
                } report2;
            };
        };
        
        struct __attribute__((packed)) {
            uint8_t reset;
        } report3;

    };
} HIDClockModeReports;


typedef struct __attribute__((packed)) {
    uint8_t reportId;
    uint8_t reportReason;
    union {
        struct __attribute__((packed)) {
            uint8_t pieceNewRow;
            uint8_t pieceNewCol;
        };
        struct __attribute__((packed)) {
            uint8_t secondPieceRow;
            uint8_t secondPieceCol;
        };
    };
    uint64_t padding;
    uint64_t padding2;
    uint64_t padding3;
    uint32_t padding4;

} HIDFrontendDataReports;

typedef struct __attribute__((packed)) {
    uint8_t reportId;
    uint8_t errorLights[8];
} HIDFrontendDataErrorReport;