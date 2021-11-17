
#ifndef HELPERFUNCTIONS_H_
#define HELPERFUNCTIONS_H_

uint8_t hexToInt(char c);
uint32_t stringHexToInteger(char *s);
void printTwoBytesHex(uint16_t number);
void printHex(uint32_t hexNumber);
uint32_t getR0FromPSP(uint32_t *PSP);
uint32_t getR1FromPSP(uint32_t *PSP);
uint8_t getSVCNumber(uint32_t *PSP);
int myAtoi(char *s);
bool stringsEqual(char *str1, char *str2);
char* intToString(int num);

#endif
