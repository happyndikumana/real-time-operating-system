// Shell Interface
// Happy Ndikumana

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz1

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "clock.h"
#include "uart0.h"
#include "tm4c123gh6pm.h"
#include "shell.h"
#include "helperFunctions.h"

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

void getsUart0(USER_DATA* data)
{
    uint8_t count = 0;

    while(count <= MAX_CHARS)
    {
        if(count == MAX_CHARS)
        {
            data->buffer[count+1] = '\0';
        }
        char input = getcUart0();

        if(input >= 32 && input != 127)
        {
            data->buffer[count] = input;
            count++;
        }
        if((input == 8 || input == 127) && count > 0)
        {
            count--;
        }
        if(input == 13)
        {
            data->buffer[count] = '\0';
            return;
        }
    }
    return;
}
int isAlpha(char c)
{
    if((c > 57 && c < 127) || (c > 32 && c < 48))
        return 1;
    return 0;
}
int isNum(char c)
{
    if(c > 47 && c < 58)
        return 1;
    return 0;
}
void parseFields(USER_DATA* data)
{
    //loop until null terminator
    //bool firstSeen Var = true when buffer[i] != num | alpha

    uint8_t i = 0;
    int seen = 0;
    data->fieldCount = 0;
    while(data->buffer[i] != '\0') //read until null terminator. Don't read the whole buffer
    {
        if(isAlpha(data->buffer[i]) || isNum(data->buffer[i]))
        {
            if(seen == 0) //new word or number
            {
                data->fieldPosition[data->fieldCount] = i;
                if(data->buffer[i+1] != '\0') {
                    if(isNum(data->buffer[i]) && isNum(data->buffer[i+1]))
                    {
                        data->fieldType[data->fieldCount] = 'n';
                    }
                    else
                    {
                        data->fieldType[data->fieldCount] = 'a';
                    }
                } else {
                    if(isNum(data->buffer[i]))
                    {
                        data->fieldType[data->fieldCount] = 'n';
                    }
                    else
                    {
                        data->fieldType[data->fieldCount] = 'a';
                    }
                }
                data->fieldCount++;

            }
            seen = 1;
        }
        else
        {
            seen = 0;
            data->buffer[i] = '\0';
        }
        if(data->fieldCount == MAX_FIELDS)
            return;
        i++;
    }
}

bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    uint8_t i = 0;
    char * temp = &data->buffer[data->fieldPosition[0]];
    for(i = 0; temp[i] != '\0'; i++)
    {
        if(strCommand[i] != temp[i])
            return false;
    }
    if(data->fieldCount <= minArguments)
        return false;

    return true;
}
bool isValidCommand(USER_DATA* data)
{
    char commands[][10] = {"reboot", "ps", "ipcs", "kill", "pi", "preempt", "sched", "pidof"};
    char * argumentOne = &data->buffer[data->fieldPosition[0]];
    uint8_t argumentNumber = data->fieldCount;
    uint8_t i = 0;
    for(i = 0; i < 9; i++) {
        if(stringsEqual(commands[i], argumentOne))
            return true;
    }
    if(argumentNumber == 2) {
        char * argumentTwo = &data->buffer[data->fieldPosition[1]];
        if(stringsEqual(argumentTwo, "&"))
            return isAThread(argumentOne);
    }
    return false;
}
bool isAThread(char *s){
    char tasks[][10] = {"Idle", "LengthyFn", "Flash4Hz", "OneShot", "ReadKeys", "Debounce", "Important", "Uncoop", "Errant", "Shell"};
    uint8_t i = 0;
    for(i = 0; i < 11; i++) {
        if(stringsEqual(s, tasks[i])){
            return true;
        }
    }
    return false;
}

char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if(data->fieldType[fieldNumber] == 'a')
        return &data->buffer[data->fieldPosition[fieldNumber]];

    return 0;
}

int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if(data->fieldType[fieldNumber] == 'n')
    {
        char * temp = &data->buffer[data->fieldPosition[fieldNumber]];
        int32_t sum = 0;
        int i = 0;
        while(temp[i] != '\0')
        {
            int32_t currentInteger = temp[i] - 48;
            sum = (sum * 10) + currentInteger;
            i++;
        }
        return sum;
    }
    return 0;
}
//void reboot() {
//    __asm(" SVC #12");
//}
//void ps() {
//    putsUart0("PS called\n");
//}
//void ipcs() {
//    putsUart0("ipcs called\n");
//}
//void killPid(uint32_t pid) {
//    __asm(" SVC #14");
//}
//void pi(bool on) {
//    if(on)
//        putsUart0("pi ON\n");
//    else
//        putsUart0("pi OFF\n");
//}
//void preempt(uint8_t on) {
//    __asm(" SVC #11");
//}
//void sched(uint8_t prio_on) {
//    __asm(" SVC #10");
//}
//void getPidof(uint32_t *pid, char *name) {
//    __asm(" SVC #13");
//}
//void pidof(char name[]) {
//    uint32_t *pid;
//    getPidof(pid, name);
//    if(*pid != 0xFFFFFFFF) {
//        printTwoBytesHex((uint16_t)*pid);
//        putsUart0("\n");
//    }
//    else {
//        putsUart0("no matching process found\n");
//    }
//}
//void proc_name(char *s) {
//    __asm(" SVC #15");
//}
void executeInstruction(USER_DATA* data) {
    char * argumentOne = &data->buffer[data->fieldPosition[0]];
    uint8_t argumentNumber = data->fieldCount;
    if(argumentNumber == 1) {
        if(stringsEqual(argumentOne, "reboot"))
            reboot();
        else if(stringsEqual(argumentOne, "ps"))
            ps();
        else if(stringsEqual(argumentOne, "ipcs"))
            ipcs();
        else
            putsUart0("Invalid command\n");
    } else if(argumentNumber == 2) {
        char * argumentTwo = &data->buffer[data->fieldPosition[1]];
        if(stringsEqual(argumentOne, "kill")) {
//            uint32_t pid = (uint32_t)myAtoi(argumentTwo);
            uint32_t pid = stringHexToInteger(argumentTwo);
            if(pid != 0xFFFFFFFF)
                killPid(pid);
            else
                putsUart0("input was not hex\n");
        }
        else if(stringsEqual(argumentOne, "pi")) {
            if(stringsEqual(argumentTwo, "ON"))
                pi(true);
            else if (stringsEqual(argumentTwo, "OFF"))
                pi(false);
            else
                putsUart0("Invalid command\n");
        }
        else if(stringsEqual(argumentOne, "preempt")) {
            if(stringsEqual(argumentTwo, "ON"))
                preempt(1);
            else if (stringsEqual(argumentTwo, "OFF"))
                preempt(0);
            else
                putsUart0("Invalid command\n");
        }
        else if(stringsEqual(argumentOne, "sched")) {
            // PRIO | RR
            //    1 | 0
            if(stringsEqual(argumentTwo, "PRIO"))
                sched(1);
            else if (stringsEqual(argumentTwo, "RR"))
                sched(0);
            else
                putsUart0("Invalid command\n");
        }
        else if(stringsEqual(argumentOne, "pidof"))
            pidof(argumentTwo);
        else if(stringsEqual(argumentTwo, "&"))
            proc_name(argumentOne);
        else
            putsUart0("Invalid command\n");
    } else {
        putsUart0("Invalid command\n");
    }
}
//#define DEBUG

void shellFunction(void) {
    USER_DATA data;
        while(1)
        {
            putsUart0("> ");

            // Get the string from the user
            getsUart0(&data);

            // Echo back to the user of the TTY interface for testing
            #ifdef DEBUG
            putsUart0(data.buffer);
            putcUart0('\n');
            #endif

            // Parse fields
            parseFields(&data);
            // Echo back the parsed field data (type and fields)
            #ifdef DEBUG
            uint8_t i;
            for (i = 0; i < data.fieldCount; i++)
            {
                putcUart0(data.fieldType[i]);
                putcUart0('\t');
                putsUart0(&data.buffer[data.fieldPosition[i]]);
                putcUart0('\n');
            }
            #endif
            // Command evaluation
            // set add, data → add and data are integers
            if(isValidCommand(&data))
                executeInstruction(&data);
//                putsUart0("valid command\n");
            else
                putsUart0("Invalid command\n");
        }
}
//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------
//int main(void)
//{
//    initHw();
//    initUart0();
//    setUart0BaudRate(115200, 40e6);
//    shell();
//
//}
