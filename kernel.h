

#ifndef KERNEL_H_
#define KERNEL_H_

#define SRAM 0x20000000
#define REGION_SIZE 0x2000
#define SRD_MASK 0x0000FF00
#define REGION_2 0x20000000
#define REGION_3 0x20002000
#define REGION_4 0x20004000
#define REGION_5 0x20006000

void setPSP();
uint32_t getPSP();
uint32_t getMSP();
void setASPBit();
void setTMPLBit();
void enableMPUIsr();
void enableBusFaultIsr();
void enableUsageFaultIsr();
void clearMPUFaultBit();
void triggerPENDSVIsrCall();
void enableMPU();
void setBaseRegion();
void setFlashRegion();
void setSRAMRegion();
uint32_t getRegionAddress(uint32_t address);
void setSrd(uint32_t srd);
void disableSubRegion(uint8_t baseAddress, uint32_t SRD);

#endif
