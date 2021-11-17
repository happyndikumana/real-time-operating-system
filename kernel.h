

#ifndef KERNEL_H_
#define KERNEL_H_

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
void disableSubRegion(uint32_t baseAddress, uint32_t SRD);

#endif
