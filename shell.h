#ifndef SHELL_H_
#define SHELL_H_

#define MAX_CHARS 80
#define MAX_FIELDS 5
#define COMMAND_NUM 9
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

void getsUart0(USER_DATA* data);
int isAlpha(char c);
int isNum(char c);
void parseFields(USER_DATA* data);
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);
bool isValidCommand(USER_DATA* data);
bool isAThread(char *s);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
void executeInstruction(USER_DATA* data);
void shellFunction();

#endif
