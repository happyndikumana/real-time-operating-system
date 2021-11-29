#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

uint32_t getR0FromPSP(uint32_t *PSP) {
    return *PSP;
}
uint32_t getR1FromPSP(uint32_t *PSP) {
    return *(PSP + 1);
}
uint8_t getSVCNumber(uint32_t *PSP) {
    /*
     * PSP is pointing at R0, so we must move it up 6 places to point to PC.
     * if svc was called from sleep, it was the only instruction in that function
     *      so PC contains the instruction right after the svc command.
     * svc command is a uint 16 bit. 8 higher bit contain the svc command and the lower 8 bits contain the number N
     *      so by casting the contents of PC to a uint8_t*, we can decrement it by two to get to the svc instruction.
     * To get the N number, dereference the address and save the lower 8 bits of the contents by casting it to uint8_t
     */
    uint32_t *PC = (uint32_t*)*(PSP+6);
    uint8_t *svcCommand = (uint8_t*)PC - 2;
    uint8_t N = (uint8_t)(*svcCommand);
    return N;
}
void printState(uint8_t state) {
    if(state == 0) {
        putsUart0("INVALID");
    } else if(state == 1) {
        putsUart0("UNRUN");
    } else if(state == 2) {
        putsUart0("READY");
    } else if(state == 3) {
        putsUart0("DELAYED");
    } else if(state == 4) {
        putsUart0("BLOCKED");
    } else {
        putsUart0("SUSPENDED");
    }
}
void printByteHex(uint8_t number) {
    char hexadecimals[16] = "0123456789ABCDEF";
    char printableHex[5];
    printableHex[0] = '0';
    printableHex[1] = 'x';
    uint8_t i = 2;
    uint8_t shifter = 4;
    uint8_t ANDNum = 0xF0;
    while(ANDNum != 0) {
        uint8_t fourBits = ((number & ANDNum) >> shifter);
        printableHex[i++] = hexadecimals[fourBits];
        ANDNum = (ANDNum >> 4);
        shifter -= 4;
    }
    printableHex[4] = '\0';
    putsUart0(printableHex);
}
void printTwoBytesHex(uint16_t number) {
    char hexadecimals[16] = "0123456789ABCDEF";
    char printableHex[7];
    printableHex[0] = '0';
    printableHex[1] = 'x';
    uint8_t i = 2;
    uint8_t shifter = 12;
    uint16_t ANDNum = 0xF000;
    while(ANDNum != 0) {
        uint8_t fourBits = ((number & ANDNum) >> shifter);
        printableHex[i++] = hexadecimals[fourBits];
        ANDNum = (ANDNum >> 4);
        shifter -= 4;
    }
    printableHex[6] = '\0';
    putsUart0(printableHex);
}

void printHex(uint32_t hexNumber) {
    char hexadecimals[16] = "0123456789ABCDEF";
    char printableHex[11];
    printableHex[0] = '0';
    printableHex[1] = 'x';
    uint8_t i = 2;
    uint8_t shifter = 28;
    uint32_t ANDNum = 0xF0000000;
    while(ANDNum != 0) {
        uint32_t fourBits = ((hexNumber & ANDNum) >> shifter);
        printableHex[i++] = hexadecimals[fourBits];
        ANDNum = (ANDNum >> 4);
        shifter -= 4;
    }
    printableHex[10] = '\0';
    putsUart0(printableHex);
}

uint8_t hexToInt(char c) {
    if(c > 47 && c < 58)
        return (uint8_t)(c - 48);
    else if(c > 64 && c < 71)
        return (uint8_t)((c - 65) + 10);
    else
        return (uint8_t)((c - 97) + 10);
}
uint32_t stringHexToInteger(char *s) {
    //TODO: check first two chars if they are '0' and 'x', respectively
    if(s[0] != '0' || (s[1] != 'x' && s[1] != 'X'))
        return -1;
    uint8_t i = 2;
    uint32_t hexNumber = 0;
    while(s[i] != '\0'){
        if((s[i] > 47 && s[i] < 58) || (s[i]) || (s[i] > 96 && s[i] < 103)) {
            uint8_t currentNum = hexToInt(s[i]);
            hexNumber |= currentNum;
            if(s[i+1] != '\0')
                hexNumber <<= 4;
        } else {
            return -1;
        }
        i++;
    }
    return hexNumber;
}

int myAtoi(char *s)
{
    //discard all white spaces: Loop over the string until current character is not space
    //go to the next char & check if the character is a negative, positive or a number.
    //create an int sign = 1. if the current char== neg -> sign = -1 and incr iterator; if current char==pos -> sign = 1 and incr iterator. if s[0] is not an int or a sign, return 0;
        //loop over the string while s[i] is a number, break if the current sum> INT_MAX or sum < INT_MIN.

        char temp[50];
        int i = 0;
        int sign = 1;
        long sum = 0;
        while(s[i] == ' ') {
            i++;
        }
        if(s[i] == '-'|| s[i] =='+'|| (s[i] > 47 && s[i] < 58)) {
            if(s[i] == '-') {
                sign = -1;
                i++;
            } else if(s[i] == '+') {
                sign = 1;
                i++;
            }
        } else {
            return 0;
        }
        while(s[i] > 47 && s[i] < 58) {
            int currentInteger = s[i] - 48;
            sum = (sum * 10) + currentInteger;
            if(sum > 2147483646)
                break;
            else if (sum < -2147483647)
                break;
            i++;
        }
    sum = sum * sign;
    return sum;
}

bool stringsEqual(char *str1, char *str2) {
    uint8_t str1Length = 0;
    uint8_t str2Length = 0;
    //kill
    //str1Length = 0,1,2,3,4
    while(str1[str1Length] != '\0')
        str1Length++;
    while(str2[str2Length] != '\0')
        str2Length++;
    if(str1Length != str2Length)
        return false;
    uint8_t i = 0;
    for(i = 0; i < str1Length; i++)
    {
        if(str1[i] != str2[i])
            return false;
    }
    return true;
}
void printNumberPercentage(uint32_t num) {
    char number[10];
    int8_t i = 0;
    while(num != 0) {
        uint8_t n = num % 10;
        num = num / 10;
        if(i == 2) {
            number[i] = '.';
            i++;
        }
        number[i] = '0' + n;
        i++;
    }
    while(i <= 3) {
        if(i == 2) {
            number[i] = '.';
            i++;
        }
        number[i] = '0';
        i++;
    }
    i--;
    for(; i >= 0; i--) {
        putcUart0(number[i]);
    }
}
char* intToString(int num) {
    char temp[10];
    char s[10];
    if(num == 0){
        s[0] = '0';
        s[1] = '\0';
        return s;
    }
    uint8_t i = 0;
    while(num != 0) {
        uint8_t n = num % 10;
        temp[i] = 48 + n;
        num /= 10;
        i++;
    }
//    temp[i] = '\0';
    uint8_t length = i;
    i -= 1;
    uint8_t j = 0;
    for(j = 0; j < length; j++) {
        s[j] = temp[i];
        i--;
    }
    s[j] = '\0';
    return (char*)s;
}
void reStartTimer1() {
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;
    TIMER1_TAV_R = 0;
    TIMER1_CTL_R |= TIMER_CTL_TAEN;
}
uint32_t getTime() {
    return (uint32_t)TIMER1_TAV_R;
}

