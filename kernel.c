#include <stdint.h>
#include "kernel.h"
#include "tm4c123gh6pm.h"

    /*
     * CTRL register
     *  set the ENABLE bit to enable the MPU
     *  disable the PRIVDEFEN bit to disable use of default memory map.
     *  set the HFNMIENA bit to enable MPU during Hard fault, NMI and FAULTMASK handlers
     * MPUBASE
     *  set base address of MPU region
     *  enable the valid bit to make sure MPUNUMBER gets updated with new region number
     *  set the region number
     * MPUATTR
     *  set the ENABLE bit to enable the region specified by the MPUNUMBER register
     *  set the SIZE of the region
     *  if it's flash, set the SRD bit to disable sub-regions
     *  set appropriate XN, AP and TEX bits
     */

void setPSP(uint32_t PSP) {
    __asm(" MSR  PSP, R0");
}
uint32_t getPSP(){
    __asm(" MRS  R0, PSP");
}
uint32_t getMSP(){
    __asm(" MRS  R0, MSP");
}
void setASPBit() {
    __asm(" MRS R1, CONTROL");
    __asm(" ORR R1, #2");
    __asm(" MSR CONTROL, R1");
    __asm(" ISB");
}
void setTMPLBit() {
    __asm(" MRS R1, CONTROL");
    __asm(" ORR R1, #1");
    __asm(" MSR CONTROL, R1");
    __asm(" ISB");
}
void enableMPUIsr() {
    NVIC_SYS_HND_CTRL_R |= 0x00010000;
}
void enableBusFaultIsr() {
    NVIC_SYS_HND_CTRL_R |= 0x00020000;
}
void enableUsageFaultIsr(){
    NVIC_SYS_HND_CTRL_R |= 0x00030000;
}
void clearMPUFaultBit(){
    NVIC_SYS_HND_CTRL_R &= ~(0x00002000);
}
void triggerPENDSVIsrCall(){
    NVIC_INT_CTRL_R |= 0x10000000;
}

void enableMPU() {
    NVIC_MPU_CTRL_R |= 0x5; //0111
}
void setBaseRegion() {
    //start add = 0
    //size = 4GB
    NVIC_MPU_BASE_R |= 0x00000010; //region 0
    NVIC_MPU_ATTR_R |= 0x1307003F; //0001 0 011 0000 0111 0000 0000 0011 1111
                                   //       AP = 011 = RW/RW
}
void setFlashRegion() {
    //start add: 0x0000 0000
    //size: 256 KB
    NVIC_MPU_BASE_R |= 0x00000011; // region 1
    NVIC_MPU_ATTR_R |= 0x03020023;
}

void setSRAMRegion() {
    //start add: 0x2000 0000
    //size: 32KB/4 per region
    //  region 2 start add:0x2000 0000
    //  size: 12
    //  region 2 start add:0x2000 2000
    //  size: 12
    //  region 2 start add:0x2000 4000
    //  size: 12
    //  region 2 start add:0x2000 6000
    //  size: 12

    //region 2
    NVIC_MPU_BASE_R |= 0x20000012; // region 2
    NVIC_MPU_ATTR_R |= 0x01060019;
    //region 3
    NVIC_MPU_BASE_R |= 0x20002013; // region 3
    NVIC_MPU_ATTR_R |= 0x01060019;
    //region 4
    NVIC_MPU_BASE_R |= 0x20004014; // region 4
    NVIC_MPU_ATTR_R |= 0x01060019;
    //region 5
    NVIC_MPU_BASE_R = 0x20006015; // region 5
    NVIC_MPU_ATTR_R = 0x01060019;
}

void disableSubRegion(uint32_t regionNumber, uint32_t SRD) {
    NVIC_MPU_NUMBER_R = regionNumber; // region 2
    NVIC_MPU_ATTR_R |= SRD;
}
//after set up. set TMPL to be 1 p.88
