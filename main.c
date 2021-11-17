//#include <stdio.h>
//#include "tm4c123gh6pm.h"
//#include "clock.h"
//#include "gpio.h"
//#include "uart0.h"
//
//#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) // on-board blue LED
//#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4))) // off-board red LED
//#define ORANGE_LED   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 3*4))) // off-board orange LED
//#define YELLOW_LED   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 4*4))) // off-board yellow LED
//#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 0*4))) // off-board green LED
//
//
//#define START_HEAP_ADDRESS 0x20001400
//
//uint32_t* heap = START_HEAP_ADDRESS;
//
////void unlockPortD() {
////    GPIO_PORTD_LOCK_R = 0x4C4F434B;
////    GPIO_PORTD_CR_R |= 0xFF;
////}
//void initHw() {
//    initSystemClockTo40Mhz();
//    enablePort(PORTA);
//    enablePort(PORTC);
//    enablePort(PORTD);
//    enablePort(PORTE);
//    enablePort(PORTF);
//
//    //LEDs
//    selectPinPushPullOutput(PORTA,2); //Red
//    selectPinPushPullOutput(PORTA,3); //Orange
//    selectPinPushPullOutput(PORTA,4); //Yellow
//
//    selectPinPushPullOutput(PORTE,0); //green
//
//    selectPinPushPullOutput(PORTF,2); //blue
//
//
//    //Push Buttons
//    selectPinDigitalInput(PORTC, 4);    //PB5   0010 0000
//    selectPinDigitalInput(PORTC, 5);    //PB4   0001 0000
//    selectPinDigitalInput(PORTC, 6);    //PB3   0000 1000
//    selectPinDigitalInput(PORTC, 7);    //PB2   0000 0100
////    unlockPortD();
//    setPinCommitControl(PORTD, 7);
//
//    selectPinDigitalInput(PORTD, 6);    //PB1   0000 0010
//    selectPinDigitalInput(PORTD, 7);    //PB0   0000 0001
//
//    enablePinPullup(PORTC, 4);
//    enablePinPullup(PORTC, 5);
//    enablePinPullup(PORTC, 6);
//    enablePinPullup(PORTC, 7);
//    enablePinPullup(PORTD, 6);
//    enablePinPullup(PORTD, 7);
//
//
//}
//uint8_t readPbs() {
//    uint8_t sum = 0;
//
//    if(getPinValue(PORTD, 7) == 0)
//        sum += 1;
//    if(getPinValue(PORTD, 6) == 0)
//        sum += 2;
//    if(getPinValue(PORTC, 7) == 0)
//        sum += 4;
//    if(getPinValue(PORTC, 6) == 0)
//        sum += 8;
//    if(getPinValue(PORTC, 5) == 0)
//        sum += 16;
//    if(getPinValue(PORTC, 4) == 0)
//        sum += 32;
//    return sum;
//}
//int main(void)
//{
//    initHw();
//    initUart0();
//
//    char buffer[20];
//    sprintf(buffer,"Heap = %p\n", heap);
//    putsUart0(buffer);
//
//    BLUE_LED = 1;
//    waitMicrosecond(1000000);
//    RED_LED = 1;
//    waitMicrosecond(1000000);
//    ORANGE_LED = 1;
//    waitMicrosecond(1000000);
//    YELLOW_LED = 1;
//    waitMicrosecond(1000000);
//    GREEN_LED = 1;
//
//    waitMicrosecond(2000000);
//
//    setPinValue(PORTA, 2, 0);
//    setPinValue(PORTA, 3, 0);
//    setPinValue(PORTA, 4, 0);
//    setPinValue(PORTE, 0, 0);
//    setPinValue(PORTF, 2, 0);
//
//    waitMicrosecond(2000000);
//
//    setPinValue(PORTA, 2, 1);
//    setPinValue(PORTA, 3, 1);
//    setPinValue(PORTA, 4, 1);
//    setPinValue(PORTE, 0, 1);
//    setPinValue(PORTF, 2, 1);
//
//    waitMicrosecond(2000000);
//
//    setPinValue(PORTA, 2, 0);
//    setPinValue(PORTA, 3, 0);
//    setPinValue(PORTA, 4, 0);
//    setPinValue(PORTE, 0, 0);
//    setPinValue(PORTF, 2, 0);
//
//    waitMicrosecond(2000000);
//
//    setPinValue(PORTA, 2, 1);
//    setPinValue(PORTA, 3, 1);
//    setPinValue(PORTA, 4, 1);
//    setPinValue(PORTE, 0, 1);
//    setPinValue(PORTF, 2, 1);
//
//    waitMicrosecond(2000000);
//
//    setPinValue(PORTA, 2, 0);
//    setPinValue(PORTA, 3, 0);
//    setPinValue(PORTA, 4, 0);
//    setPinValue(PORTE, 0, 0);
//    setPinValue(PORTF, 2, 0);
//
//    int i = 0;
//    for(i = 0; i <= 64; i++) {
//        uint8_t sum = readPbs();
//        sprintf(buffer, "sum = %hhu\n", sum);
//        putsUart0(buffer);
//        waitMicrosecond(2000000);
//    }
//
//
//
//	while(1);
//}
