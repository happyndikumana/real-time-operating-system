
#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_


uint8_t hexToInt(char c);
void printState(uint8_t state);
uint32_t stringHexToInteger(char *s);
void printByteHex(uint8_t number);
void printTwoBytesHex(uint16_t number);
void printHex(uint32_t hexNumber);
uint32_t getR0FromPSP(uint32_t *PSP);
uint32_t getR1FromPSP(uint32_t *PSP);
uint8_t getSVCNumber(uint32_t *PSP);
int myAtoi(char *s);
bool stringsEqual(char *str1, char *str2);
void printNumberPercentage(uint32_t num);
char* intToString(int num);
void reStartTimer1();
uint32_t getTime();

#endif
