#include <stdint.h>
#include "stm32f411xe.h"
#include "usb.h"
#include "usbd_customhid.h"
#include "string.h"
#include "test.h"
#include "stm32f4xx_ll_usb.h"

extern HIDClockModeReports clockModeReport;
extern USBD_HandleTypeDef hUsbDeviceFS;

void sendTestGame() {

 // Move 1
uint8_t arr1[8][8] = {{1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,1,1}};
uint8_t arr2[8][8] = {{1,1,1,1,1,1,1,1},{1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,1,1}};
clockModeReport.reportId = 1;
clockModeReport.firstPickupRow = 6;
clockModeReport.firstPickupCol = 4;
clockModeReport.report1.finalPickupRow = 4;
clockModeReport.report1.finalPickupCol = 4;
// memcpy(clockModeReport.secondPickupState, arr2, 8 * 8 * sizeof(arr2[0][0]));
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 5);
HAL_Delay(100);

// Move 2
uint8_t arr3[8][8] = {{1,1,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,1,1}};
uint8_t arr4[8][8] = {{1,1,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,1,1}};
clockModeReport.reportId = 1;
clockModeReport.firstPickupRow = 1;
clockModeReport.firstPickupCol = 4;
clockModeReport.report1.finalPickupRow = 3;
clockModeReport.report1.finalPickupCol = 4;
// memcpy(clockModeReport.thirdPickupState, arr4, 8 * 8 * sizeof(arr2[0][0]));
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 5);
HAL_Delay(100);

// Move 3
uint8_t arr5[8][8] = {{1,1,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
uint8_t arr6[8][8] = {{1,1,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,1,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
clockModeReport.reportId = 1;
clockModeReport.firstPickupRow = 7;
clockModeReport.firstPickupCol = 6;
clockModeReport.report1.finalPickupRow = 5;
clockModeReport.report1.finalPickupCol = 5;
// memcpy(clockModeReport.thirdPickupState, arr6, 8 * 8 * sizeof(arr2[0][0]));
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 5);

HAL_Delay(100);

// Move 4
uint8_t arr7[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,1,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
uint8_t arr8[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,1,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,1,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
clockModeReport.reportId = 1;
clockModeReport.firstPickupRow = 0;
clockModeReport.firstPickupCol = 1;
clockModeReport.report1.finalPickupRow = 2;
clockModeReport.report1.finalPickupCol = 2;
// memcpy(clockModeReport.thirdPickupState, arr8, 8 * 8 * sizeof(arr2[0][0]));
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 5);
HAL_Delay(100);

// Move 5
uint8_t arr9[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,1,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,1,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
uint8_t arr10[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,1,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
uint8_t arr11[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,1,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
clockModeReport.reportId = 2;
clockModeReport.firstPickupRow = 3;
clockModeReport.firstPickupCol = 4;
clockModeReport.report2.secondPickupRow = 5;
clockModeReport.report2.secondPickupCol = 5;
clockModeReport.report2.finalPickupRow = 8;
clockModeReport.report2.finalPickupCol = 8;
// memcpy(clockModeReport.thirdPickupState, arr11, 8 * 8 * sizeof(arr2[0][0]));
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 7);

HAL_Delay(100);

// Move 6
uint8_t arr12[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,1,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
uint8_t arr13[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
uint8_t arr14[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,1,0,1}};
clockModeReport.reportId = 2;
clockModeReport.firstPickupRow = 2;
clockModeReport.firstPickupCol = 2;
clockModeReport.report2.secondPickupRow = 3;
clockModeReport.report2.secondPickupCol = 4;
clockModeReport.report2.finalPickupRow = 8;
clockModeReport.report2.finalPickupCol = 8;
// memcpy(clockModeReport.thirdPickupState, arr14, 8 * 8 * sizeof(arr2[0][0]));
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS,(uint32_t*)&clockModeReport, 7);

HAL_Delay(100);

clockModeReport.reportId = 3;
clockModeReport.report3.reset = 1;
USBD_CUSTOM_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&clockModeReport, 2); 

// HAL_Delay(500);
// // Move 7
// uint8_t arr15[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,0,1,1,1},{1,1,1,1,1,0,0,1}};
// uint8_t arr16[8][8] = {{1,0,1,1,1,1,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1},{1,1,1,1,1,0,0,1}};
// clockModeReport.firstPickupRow = 7;
// clockModeReport.firstPickupCol = 5;
// memcpy(clockModeReport.secondPickupState, arr16, sizeof(arr16));
// sendReport(1, 67);

// // Move 8
// uint8_t arr17[8][8] = {{1,0,1,1,1,0,1,1},{1,1,1,1,0,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1},{1,1,1,1,1,0,0,1}};
// uint8_t arr18[8][8] = {{1,0,1,1,1,0,1,1},{1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1},{1,1,1,1,1,0,0,1}};
// clockModeReport.firstPickupRow = 0;
// clockModeReport.firstPickupCol = 5;
// clockModeReport.secondPickupRow = 1;
// clockModeReport.secondPickupCol = 4;
// memcpy(clockModeReport.thirdPickupState, arr18, sizeof(arr18));
// sendReport(2, 69);

// // Move 9
// uint8_t arr19[8][8] = {{1,0,1,1,1,0,1,1},{1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1},{1,1,1,1,0,0,0,1}};
// uint8_t arr20[8][8] = {{1,0,1,1,1,0,1,1},{1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1},{1,1,1,1,0,0,0,0}};
// uint8_t arr21[8][8] = {{1,0,1,1,1,0,1,1},{1,1,1,1,1,1,1,1},{0,0,0,0,0,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,1,0,0,0},{0,0,0,0,0,0,0,0},{1,1,1,1,1,1,1,1},{1,1,1,1,0,1,1,0}};
// clockModeReport.firstPickupRow = 7;
// clockModeReport.firstPickupCol = 4;
// clockModeReport.secondPickupRow = 7;
// clockModeReport.secondPickupCol = 7;
// memcpy(clockModeReport.thirdPickupState, arr21, sizeof(arr21));
// sendReport(2, 69)

}