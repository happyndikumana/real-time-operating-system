// RTOS Framework - Fall 2021
// J Losh

// Student Name:
// TO DO: Add your name on this line.  Do not include your ID number in the file.

// Please do not change any function name in this code or the thread priorities

//-----------------------------------------------------------------------------
// Hardware Target
//-----------------------------------------------------------------------------

// Target Platform: EK-TM4C123GXL Evaluation Board
// Target uC:       TM4C123GH6PM
// System Clock:    40 MHz

// Hardware configuration:
// 6 Pushbuttons and 5 LEDs, UART
// LEDs on these pins:
// Blue:   PF2 (on-board)
// Red:    PA2
// Orange: PA3
// Yellow: PA4
// Green:  PE0
// PBs on these pins
// PB0:    PC4
// PB1:    PC5
// PB2:    PC6
// PB3:    PC7
// PB4:    PD6
// PB5:    PD7
// UART Interface:
//   U0TX (PA1) and U0RX (PA0) are connected to the 2nd controller
//   The USB on the 2nd controller enumerates to an ICDI interface and a virtual COM port
//   Configured to 115,200 baud, 8N1
// Memory Protection Unit (MPU):
// Regions to allow 32 1kiB SRAM access (RW or none)
// Region to allow peripheral access (RW)
// Region to allow flash access (XR)

//-----------------------------------------------------------------------------
// Device includes, defines, and assembler directives
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"
#include "uart0.h"
#include "wait.h"
#include "gpio.h"
#include "kernel.h"
#include "helperFunctions.h"
#include "clock.h"
#include "shell.h"

//assembly functions definitions
extern void yieldSvcCall();
extern void sleepSvcCall();
extern void pushR4ToR11ToPsp();
extern void popR4ToR11FromPsp();
extern void pushPSP(uint32_t register);

// REQUIRED: correct these bitbanding references for the off-board LEDs
#define BLUE_LED     (*((volatile uint32_t *)(0x42000000 + (0x400253FC-0x40000000)*32 + 2*4))) // on-board blue LED
#define RED_LED      (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 2*4))) // off-board red LED
#define ORANGE_LED   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 3*4))) // off-board orange LED
#define YELLOW_LED   (*((volatile uint32_t *)(0x42000000 + (0x400043FC-0x40000000)*32 + 4*4))) // off-board yellow LED
#define GREEN_LED    (*((volatile uint32_t *)(0x42000000 + (0x400243FC-0x40000000)*32 + 0*4))) // off-board green LED

//-----------------------------------------------------------------------------
// RTOS Defines and Kernel Variables
//-----------------------------------------------------------------------------

// function pointer
typedef void (*_fn)();

// semaphore
#define MAX_SEMAPHORES 5
#define MAX_QUEUE_SIZE 5
typedef struct _semaphore
{
    uint16_t count;
    uint16_t queueSize;
    uint32_t processQueue[MAX_QUEUE_SIZE]; // store task index here
} semaphore;

semaphore semaphores[MAX_SEMAPHORES];

typedef struct _semaphoreData
{
    char processNames[MAX_QUEUE_SIZE][16];
    uint16_t count;
    uint16_t queueSize;
    uint32_t processQueue[MAX_QUEUE_SIZE]; // store task index here
} semaphoreData;

typedef struct _psData {
    uint16_t pid;
    char name[16];
    uint8_t state;
    uint32_t time;
} processData;

#define keyPressed 1
#define keyReleased 2
#define flashReq 3
#define resource 4

#define NULL 0x00000000

//heap
#define START_HEAP_ADDRESS 0x20001400

// task
#define STATE_INVALID    0 // no task
#define STATE_UNRUN      1 // task has never been run
#define STATE_READY      2 // has run, can resume at any time
#define STATE_DELAYED    3 // has run, but now awaiting timer
#define STATE_BLOCKED    4 // has run, but now blocked by semaphore
#define STATE_SUSPENDED  5 // task has been destroyed, but now waits be restarted

//svc call numbers
#define SVC_YIELD 2
#define SVC_SLEEP 3
#define SVC_WAIT  4
#define SVC_POST  5
#define SVC_PRIORITY 10
#define SVC_PREEMPTION 11
#define SVC_REBOOT 12
#define SVC_PIDOF 13
#define SVC_KILLPID 14
#define SVC_PROCESS 15
#define SVC_IPCS 16
#define SVC_PS 17
#define SVC_PI 18


#define MAX_TASKS 12       // maximum number of valid tasks
#define MAX_PRIORTIES 8    // maximum number of priorities
uint8_t taskCurrent = 0;   // index of last dispatched task
uint8_t taskCount = 0;     // total number of valid tasks
uint32_t* heap = (uint32_t*)START_HEAP_ADDRESS;
uint8_t priorityIndex;
uint8_t priorityStartIndices[MAX_PRIORTIES];
uint8_t priorityOn = 0;
uint8_t preemptionOn = 0;
uint32_t totalProcessTime = 0;
uint32_t processTimes[MAX_TASKS];

// REQUIRED: add store and management for the memory used by the thread stacks
//           thread stacks must start on 1 kiB boundaries so mpu can work correctly

struct _tcb
{
    uint8_t state;                 // see STATE_ values above
    void *pid;                     // used to uniquely identify thread
    void *spInit;                  // original top of stack
    void *sp;                      // current stack pointer
    int8_t priority;               // 0=highest to 7=lowest
    uint32_t ticks;                // ticks until sleep complete
    uint32_t srd;                  // MPU subregion disable bits
    char name[16];                 // name of task used in ps command
    void *semaphore;               // pointer to the semaphore that is blocking the thread
    uint32_t time;                   // cummulative time a task has ran
} tcb[MAX_TASKS];

//-----------------------------------------------------------------------------
// RTOS Kernel Functions
//-----------------------------------------------------------------------------

// REQUIRED: initialize systick for 1ms system timer
void initRtos()
{

    uint8_t i;
    // no tasks running
    taskCount = 0;
    // clear out tcb records
    for (i = 0; i < MAX_TASKS; i++)
    {
        tcb[i].state = STATE_INVALID;
        tcb[i].pid = 0;
        tcb[i].time = 0;
    }
    //initialize systick for 1ms system timer
    NVIC_ST_RELOAD_R = 39999;  // = systick isr rate/ system clock  = 1 ms/ 25 ns
    NVIC_ST_CTRL_R |= 0x7;     // 0111 = CLK_SRC set, INTEN set, ENABLE set

    //populating priorityStartIndices array to 0. This makes sure priority will start looking from index 0
    for(i = 0 ; i < MAX_PRIORTIES; i++){
        priorityStartIndices[i] = 0;
    }
    priorityIndex = 0;

    setBaseRegion();
    setFlashRegion();
    setSRAMRegion();
    enableBusFaultIsr();
    enableMPUIsr();
    enableMPU();
}

// REQUIRED: Implement prioritization to 8 levels
int rtosScheduler()
{
//    bool prioritized = true;
    bool ok;
    static uint8_t task = 0xFF;
//    static uint8_t i = 0;
    ok = false;
    if(!priorityOn) {
        while (!ok)
        {
            task++;
            if (task >= MAX_TASKS)
                task = 0;
            ok = (tcb[task].state == STATE_READY || tcb[task].state == STATE_UNRUN);
        }
    }

    //prioritized
    if(priorityOn) {
         while(!ok) {
            uint8_t j;
            for(j = priorityStartIndices[priorityIndex]; j < MAX_TASKS; j++) {
                int8_t priority = tcb[j].priority;
                uint8_t state = tcb[j].state;
                if(priority == priorityIndex && (state == STATE_READY || state == STATE_UNRUN)) {
                    priorityStartIndices[priorityIndex] = (j + 1) % 12;
                    task = j;
                    ok = true;
                    break;
                }
            }
            if(!ok) {
                priorityIndex = (priorityIndex + 1) % 8;
                if(priorityIndex == 0) {
                    uint8_t k;
                    for(k = 0 ; k < MAX_PRIORTIES; k++){
                        priorityStartIndices[k] = 0;
                    }
                }
            }
        }
//        task = 0;
//        task++;
//        while (!ok) {
//            uint8_t state = tcb[priorityStartIndices[i]].state;
//            int8_t priority = tcb[priorityStartIndices[i]].priority;
//            ok = (state == STATE_READY | STATE_UNRUN) & (priority == i);
//            if(!ok) {
//                task++;
//                priorityStartIndices[i] = (priorityStartIndices[i] + 1) % 12;
//                if(task == 12) {
//                    i++;
//                    task = 0;
//                }
//            } else {
//                priorityStartIndices[i] = (priorityStartIndices[i] + 1) % 12;
//            }
//        }
    }

    return task;

    /*
     * Priority scheduling
     * create a priorityArrayLevel[8]
     *      each will contain the index of the tcb index that should be looked at next.
     * found = false;
     * lookover = 0 // keeps track of the tcb entries you have checked at that level
     * loop over all levels. while i <= MAX_LEVELS () (there are 8 levels) while not found
     * while(!found)
     *      find next ready or unrun tasks at level i
     *          found = tcb[next[i]].state == ready or unrun
     *      if(!found)
     *          increment lookover
     *          increment next[i]%12
     *              priorityArrayLevel[i] = (priorityArrayLevel[i] + 1)%12
     *          if(lookover == 12) //so if i've checked all tcb entries (MAX_TASKS), then move to the next level
     *              i++
     *              lookover = 0; //reset lookover
     *      if(found)
     *          exit loop
     */
}

bool createThread(_fn fn, const char name[], uint8_t priority, uint32_t stackBytes)
{
    bool ok = false;
    uint8_t i = 0;
    bool found = false;
    // REQUIRED:
    // store the thread name
    // allocate stack space and store top of stack in sp and spInit
    // add task if room in task list
    if (taskCount < MAX_TASKS)
    {
        // make sure fn not already in list (prevent reentrancy)
        while (!found && (i < MAX_TASKS))
        {
            found = (tcb[i++].pid ==  fn);
        }
        if (!found)
        {
            // find first available tcb record
            i = 0;
            while (tcb[i].state != STATE_INVALID) {i++;}
            tcb[i].state = STATE_UNRUN;
            tcb[i].pid = fn;
            //round to nearest 1KiB
            stackBytes = (((stackBytes - 1)/1024) + 1) * 1024;
            if(i == 0) {
                tcb[i].sp = (START_HEAP_ADDRESS + stackBytes) - 1;
                tcb[i].spInit = tcb[i].sp;
            } else {
                tcb[i].sp = ((tcb[i - 1].spInit + 1) + stackBytes) - 1;
                tcb[i].spInit = tcb[i].sp;
            }
            tcb[i].priority = priority;
            //calculate how may 1k regions the thread needs.
            //this will correspond to the amount of srd bits that need to be set
            uint8_t srdBitsAmount = stackBytes/1024;
            tcb[i].srd = 0;
            //transform the srd bits amount into a the correct number to load into the SRD bits of MPUATTR
            while(srdBitsAmount > 0) {
                tcb[i].srd |= 1;
                srdBitsAmount--;
                if(srdBitsAmount != 0)
                    tcb[i].srd <<= 1;
            }
            //bit shift the SRD bits into the correct subregion position
            uint32_t regionAddress = getRegionAddress((uint32_t)tcb[i].spInit);
            uint32_t shift = (uint32_t)(tcb[i].spInit - 0x20000000 - stackBytes)/1024 + 1;
            tcb[i].srd <<= shift;
            uint8_t j = 0;
            for(j = 0; name[j]!= '\0'; j++){
                tcb[i].name[j] = name[j];
            }
            tcb[i].name[j+1] = '\0';
            // increment task count
            taskCount++;
            ok = true;
        }
    }
    // REQUIRED: allow tasks switches again
    return ok;
}

// REQUIRED: modify this function to restart a thread
void restartThread(_fn fn)
{
//    printHex(fn);
//    putsUart0("\n");
    /*
     * find task in tcb
     *      if state == SUSPENDED
     *          restart thread
     *      if state == SUSPENDED
     *          display error
     */
    uint8_t i = 0;
    for(i = 0; i < MAX_TASKS; i++){
        if(tcb[i].pid == fn) {
            if(tcb[i].state == STATE_SUSPENDED) {
                tcb[i].sp = tcb[i].spInit;
                tcb[i].state = STATE_UNRUN;
                break;
            } else {
                putsUart0("Error: ");
                putsUart0(tcb[i].name);
                putsUart0(" not suspended!\n");
                break;
            }
        }
    }

}

// REQUIRED: modify this function to destroy a thread
// REQUIRED: remove any pending semaphore waiting
// NOTE: see notes in class for strategies on whether stack is freed or not
void destroyThread(_fn fn) {
//    printHex(fn);
    /*
     * find fn (pid) in tcb
     *      save index number into currentIndex
     *      set the state of the task to STATE_INVALID
     *      dereference semaphore address and access its queue.
     *      find currentIndex in queue.
     *          delete the entry by displacing everything after it one position left.
     *          decrement queueSize
     *
     */
    uint8_t currentIndex = 0;
    for(currentIndex = 0; currentIndex < MAX_TASKS; currentIndex++) {
        if(tcb[currentIndex].pid == fn) {
            tcb[currentIndex].state = STATE_SUSPENDED;
            break;
        }
    }
    uint8_t i = 0;
    semaphore *currentSem = tcb[currentIndex].semaphore;
//    uint32_t *queue = (uint32_t*)currentSem->processQueue;
    if(currentSem != NULL) {
        for(i = 0; i < MAX_QUEUE_SIZE; i++) {
            if(currentSem->processQueue[i] == currentIndex) {
                if(currentSem->queueSize == 1) {
                    currentSem->queueSize--;
                }
                else {
                    uint8_t j = currentIndex;
                    for(; j < currentSem->queueSize - 1; j++){
                        currentSem->processQueue[j] = currentSem->processQueue[j + 1];
                    }
                    currentSem->queueSize--;
                }
                break;
            }
        }
    }
}

// REQUIRED: modify this function to set a thread priority
void setThreadPriority(_fn fn, uint8_t priority)
{
}

bool createSemaphore(uint8_t semaphore, uint8_t count)
{
    bool ok = (semaphore < MAX_SEMAPHORES);
    {
        semaphores[semaphore].count = count;
    }
    return ok;
}

// REQUIRED: modify this function to start the operating system
// by calling scheduler, setting PSP, ASP bit, and PC
//S7: mark the task as ready
void startRtos()
{
    taskCurrent = (uint8_t)rtosScheduler();
    reStartTimer1();
    //S7: mark the task as "ready" before running it and after calling scheduler
    tcb[taskCurrent].state = STATE_READY;
    setPSP((uint32_t)tcb[taskCurrent].sp);
    setASPBit();
    _fn fn = (_fn)tcb[taskCurrent].pid;
    setSrd(tcb[taskCurrent].srd);
    preemptionOn = 1;
    setTMPLBit();
    fn();
}

// REQUIRED: modify this function to yield execution back to scheduler using pendsv
void yield()
{
    __asm(" SVC #2");
}

// REQUIRED: modify this function to support 1ms system timer
// execution yielded back to scheduler until time elapses using pendsv
void sleep(uint32_t tick)
{
    //step 8
    //createThreads: idle, lengthy, flash4hz
    /*
     * when sleep is called, it triggers svcIsr
     * __asm(" SVC #N"); sleepSvcCall()
     * tick will be pushed into R0 and R0 to xPSR are pushed on the stack by hardware before calling svc
     * in svcISR, N will hit the SLEEP case and
     *      tick will be extracted from R0 and saved in tcb[taskCurrent].tick
     *      the state will be changed to delayed tcb[taskCurrent].state = DELAY
     */
//    putsUart0("ticks: ");
//    printHex(tick);
//    putsUart0("\n");
//    sleepSvcCall();
    __asm(" SVC #3");
}

// REQUIRED: modify this function to wait a semaphore using pendsv
void wait(int8_t semaphore)
{
    //step 9
    //createthreads: idle, lengthy, flash4hz, oneShot
    /*
     * trigger svcISR with a different N
     * in svcISR
     *  create a new case for WAIT (if N == WAIT)
     *      get the index of the semaphore (passed in semaphore is the index of the semaphore from the semaphores array) from R0
     *      if the semaphores[index].count > 0
     *          decrement count. :count--
     *      else
     *          add the process (pid number of taskCurrent) to semaphores[index].queue and increment queue count
     *          record the address of semaphore[index] or *semaphore[index] in tcb
     *          set the state of the taskCurrent to BLOCKED
     *              tcb[taskCurrent].state = BLOCKED
     *          set pendsv
     *
     */
    __asm(" SVC #4");
}

// REQUIRED: modify this function to signal a semaphore is available using pendsv
void post(int8_t semaphore)
{
    /*
     * to test, call: readPb, debounce, idle, lengthy, flash4hz, oneshot
     * call svc with a new number N (5)
     * in svc isr
     *      if(N == 5)
     *          increment semaphores[index].count
     *          if queueSize > 0
     *              make next task in queue ready
     *              delete the task from the queue
     *              decrement queueSize
     *              decrement count
     */
    __asm(" SVC #5");
}

// REQUIRED: modify this function to add support for the system timer
// REQUIRED: in preemptive code, add code to request task switch
void systickIsr()
{
    /*
     * swicthing at 1 kHz rate
     * loop over tcb
     *  if tcb is valid
     *      if(sleeping)
     *          decrement timeout
     *          if(timeout == 0)
     *              set State to ready
     */
//    putsUart0("in systick");
    static uint16_t count = 0;
    uint8_t i = 0;
    for(i = 0 ; i < MAX_TASKS; i++) {
        if(tcb[i].state != STATE_INVALID) {
            if(tcb[i].state == STATE_DELAYED){
                tcb[i].ticks -= 1;
                if(tcb[i].ticks == 0)
                    tcb[i].state = STATE_READY;
            }
        }
    }
    count++;
    if(count == 500) {
        count = 0;
        totalProcessTime = 0;
        uint8_t i = 0;
        for(; i < MAX_TASKS; i++) {
            processTimes[i] = tcb[i].time;
            totalProcessTime += processTimes[i];
            tcb[i].time = 0;
        }
    }

    //clears the count bit in the CTRL register. That bit is only set when the systick counter has counted down to 0;
    NVIC_ST_CURRENT_R = 12;

    //switch tasks every 1 milisecond if preemption is on
    if(preemptionOn){
        triggerPENDSVIsrCall();
    }
}

// REQUIRED: in coop and preemptive, modify this function to add support for task switching
// REQUIRED: process UNRUN and READY tasks differently
//void pendSvIsr()
//{
//    //task switch happens here
//    if(YELLOW_LED == 1)
//        YELLOW_LED = 0;
//    else
//        YELLOW_LED = 1;
//}
//S7: HW push xPSR to R0 before running function

void pendSvIsr() {
    if((NVIC_FAULT_STAT_R & 0x2) || (NVIC_FAULT_STAT_R & 0x1)) {
        tcb[taskCurrent].state = STATE_SUSPENDED;
        NVIC_FAULT_STAT_R |= 0x00000003;
        putsUart0("PendSv called from MPU\n");
        putsUart0("Suspended process: ");
        printTwoBytesHex((uint16_t)tcb[taskCurrent].pid);
        putsUart0("\n");
    }

    /*
     * push R4 to R11 of the current task to PSP (stack used by Handlers)
     * save the last address the current task was pointing at (basically PSP) into the sp attribute of tcb
     * load PSP with the sp of the task that will run next
     * pop R4 to R11 of the next task (previously pushed on task switch) from the PSP
     */
    pushR4ToR11ToPsp();
    tcb[taskCurrent].sp = getPSP();
//    uint32_t *pp = (uint32_t)tcb[taskCurrent].sp;
//    uint32_t stopTime = getTime();
    tcb[taskCurrent].time = getTime();
    taskCurrent = (uint8_t)rtosScheduler();
    setSrd(tcb[taskCurrent].srd);
    reStartTimer1();

    if(tcb[taskCurrent].state == STATE_READY) {
        setPSP((uint32_t)tcb[taskCurrent].sp);
        popR4ToR11FromPsp();
    } else {
        tcb[taskCurrent].state = STATE_READY;
        setPSP((uint32_t)tcb[taskCurrent].spInit);
        pushPSP((uint32_t)0x61000000);              //xPSR
        pushPSP((uint32_t)tcb[taskCurrent].pid);    //PC
        pushPSP((uint32_t)0xFFFFFFFD);              //LR
        pushPSP((uint32_t)12);                      //R12
        pushPSP((uint32_t)3);                       //R3
        pushPSP((uint32_t)2);                       //R2
        pushPSP((uint32_t)1);                       //R1
        pushPSP((uint32_t)0);                       //R0

    }

    /*
     * S7: if task is ready, restore PSP and pop R4-R11
     * else
     *  mark task as ready
     *  set PSP to tcb[newTask].spInit
     *  push xPSR to R0 on the stack (push from xPSR to R0) 0x6100 0000
     *      create xPSR to R0 values that look like what Hardware would've pushed
     *      xPSP = reference idle1 values
     *      PC = tcb pid. make sure LSB is 0 (reading thumb)
     *      LR
     *  set LR value to the right value (0xFFFF FFFD) what's pushed on the stack depends on the LR value p.111
     */

}
//S7: HW pops xPSR to R0 after running function

// REQUIRED: modify this function to add support for the service call
// REQUIRED: in preemptive code, add code to handle synchronization primitives
void svCallIsr()
{
//    putsUart0("svc called \n");
    uint32_t *psp = getPSP();
    uint8_t N = getSVCNumber(psp);          //check getSVCNumber() for explanation

    if(N == SVC_YIELD)
        triggerPENDSVIsrCall();
    if(N == SVC_SLEEP) {
        uint32_t ticks = getR0FromPSP(psp);     //PSP pointing to R0, last pushed register from sleep function. R0 contains passed in "ticks"
        tcb[taskCurrent].ticks = ticks;
        tcb[taskCurrent].state = STATE_DELAYED;
        triggerPENDSVIsrCall();
    }
    if(N == SVC_WAIT) {
        int8_t semaphoreIndex = (int8_t)getR0FromPSP(psp);
        if(semaphores[semaphoreIndex].count > 0) {
            semaphores[semaphoreIndex].count -= 1;
        } else {
            uint16_t i = semaphores[semaphoreIndex].queueSize;
            semaphores[semaphoreIndex].processQueue[i] = taskCurrent;
            semaphores[semaphoreIndex].queueSize += 1;
            tcb[taskCurrent].semaphore = &semaphores[semaphoreIndex];
            tcb[taskCurrent].state = STATE_BLOCKED;
            triggerPENDSVIsrCall();
        }
    }
    if(N == SVC_POST) {
        int8_t semaphoreIndex = (int8_t)getR0FromPSP(psp);
        semaphores[semaphoreIndex].count += 1;
        if(semaphores[semaphoreIndex].queueSize > 0) {
            uint8_t index = semaphores[semaphoreIndex].processQueue[0];
            tcb[index].state = STATE_READY;
            semaphores[semaphoreIndex].queueSize -= 1;
            uint8_t i = 0;
            for(i = 0; i < semaphores[semaphoreIndex].queueSize - 1; i++) {
                semaphores[semaphoreIndex].processQueue[i] = semaphores[semaphoreIndex].processQueue[i + 1];
            }
            semaphores[semaphoreIndex].count -= 1;
        }
    }
    if(N == SVC_PRIORITY) {
        priorityOn = (uint8_t)getR0FromPSP(psp);
    }
    if(N == SVC_PREEMPTION) {
        preemptionOn = (uint8_t)getR0FromPSP(psp);
    }
    if(N == SVC_REBOOT) {
        //reboot
        putsUart0("rebooting\n");
        NVIC_APINT_R = 0x05FA0000 | NVIC_APINT_SYSRESETREQ;
    }
    if(N == SVC_PIDOF) {
        uint32_t *pid = getR0FromPSP(psp);
        char *name = getR1FromPSP(psp);
        uint8_t i = 0;
        for(i = 0; i < MAX_TASKS; i++){
            if(stringsEqual(name, tcb[i].name)) {
                *pid = (uint32_t)tcb[i].pid;
                return;
            }
        }
        *pid = - 1;
    }
    if(N == SVC_KILLPID) {
        uint32_t pid = getR0FromPSP(psp);
        if(pid != 0xFFFFFFFF) {
//            printTwoBytesHex((uint16_t)pid);
//            putsUart0("\n");
            destroyThread((_fn)pid);
        }
        else
            return;
    }
    if(N == SVC_PROCESS) {
        char *s = getR0FromPSP(psp);
//        putsUart0(s);
        uint8_t i = 0;
        for(i = 0; i < MAX_TASKS; i++) {
            if(stringsEqual(tcb[i].name, s)) {
                restartThread((_fn)tcb[i].pid);
            }
        }
    }
    if(N == SVC_IPCS) {
        uint32_t *address = (uint32_t*)getR1FromPSP(psp);
        uint32_t count = 0;
        semaphoreData *semaphoreArray = (semaphore*)getR0FromPSP(psp);
        uint8_t i = 0;
        for(i = 0; i < MAX_SEMAPHORES - 1; i++) {
            uint8_t j = 0;
            semaphoreData current;
            current.count = semaphores[i+1].count;
            current.queueSize = semaphores[i+1].queueSize;
            for(j = 0; j < current.queueSize; j++) {
                uint8_t index = semaphores[i+1].processQueue[j];
                current.processQueue[j] = index;
                uint8_t k = 0;
                for(k = 0; tcb[index].name[k] != '\0'; k++){
                    current.processNames[j][k] = tcb[index].name[k];
                }
                current.processNames[j][k] = '\0';
            }
            semaphoreArray[i] = current;
            count++;
        }
        *address = count;
    }
    if(N == SVC_PS) {
        uint32_t *address = (uint32_t*)getR1FromPSP(psp);
        uint32_t count = 0;
        processData *psData = (processData*)getR0FromPSP(psp);
        uint8_t i = 0;
        for(; i < MAX_TASKS; i++) {
            if(tcb[i].pid != NULL) {
                processData current;
                current.pid = (uint16_t)tcb[i].pid;
                current.state = tcb[i].state;
//                current.time = 10; //tcb[i].time;
//                current.time = processTimes[i];
                current.time = (processTimes[i] * 100 * 100)/ totalProcessTime;
                uint8_t k = 0;
                for(k = 0; tcb[i].name[k] != '\0'; k++){
                    current.name[k] = tcb[i].name[k];
                }
                current.name[k] = '\0';
                psData[i] = current;
                count++;
            }
        }
        *address = count;
    }


    /*
     * before svc is called, HW pushes xPSR to R0
     *  if sleep(time) called svc, then time is in R0
     *
     *  Fucntions to write:
     *      getR0FromPSP() - for time
     *      getSVCNumber() - for SVC #N
     */

    /*
     * extract svc number (N)
     *  to access SVC instruction, do PC (casted as an 8 bit number) - 2
     *      #N is the lower 8 bits at that address. (dereference it)
     * if N == YIELD
     *  set pendsv bit
     * if N == SLEEP
     *  record timeout in ms. save it in tcb
     *  change state to DELAY
     *  set pendsv bit
     * if (N == WAIT)
     *      get the index of the semaphore (passed in semaphore is the index of the semaphore from the semaphores array) from R0
     *      if the semaphores[index].count > 0
     *          decrement count. :count--
     *      else
     *          add the process (pid number of taskCurrent) to semaphores[index].queue and increment queue count
     *          record the address of semaphore[index] or *semaphore[index] in tcb
     *          set the state of the taskCurrent to BLOCKED
     *              tcb[taskCurrent].state = BLOCKED
     *          set pendsv
     * if(N == 5)
     *      increment semaphores[index].count
     *      if queueSize > 0
     *           make next task in queue ready
     *           delete the task from the queue
     *           decrement queueSize
     *           decrement count
     */
}

// REQUIRED: code this function
void mpuFaultIsr(){
    //clear MPU fault

    //check MMARV bit to know if the address in faultAddress (MMADDR register) is valid
    //if MMARV == 1, address in MMADDR register is valid
    uint32_t *faultFlags = NVIC_FAULT_STAT_R; //0xE000ED28
    uint32_t *PSPAdd = getPSP();
    uint32_t *MSPAdd = getMSP();
    uint32_t faultAddress = NVIC_MM_ADDR_R; //0xE000ED34

    /*
     * print PSP
     * print MSP
     * print flags
     * print fault address
     * print stack dump
     */
    putsUart0("\n");
    putsUart0("MPU fault in process ");
    printTwoBytesHex((uint16_t)tcb[taskCurrent].pid);
    putsUart0("\n");
    putsUart0("PSP: ");
    printHex((uint32_t)*PSPAdd);
    putsUart0("\n");
    putsUart0("MSP: ");
    printHex((uint32_t)*MSPAdd);
    putsUart0("\n");
    putsUart0("NVIC_FAULT_STAT_R: ");
    printHex((uint32_t)*faultFlags  & 0xFF);
    putcUart0('\n');
    putsUart0("NVIC_MM_ADDR_R: ");
    printHex(faultAddress);
    putsUart0("\n\n");

    putsUart0("Stack dump:\n\n");
    putsUart0("R0\t= ");
    printHex((uint32_t)*PSPAdd);
    putcUart0('\n');
    putsUart0("R1\t= ");
    printHex((uint32_t)*(PSPAdd+1));
    putcUart0('\n');
    putsUart0("R2\t= ");
    printHex((uint32_t)*(PSPAdd+2));
    putcUart0('\n');
    putsUart0("R3\t= ");
    printHex((uint32_t)*(PSPAdd+3));
    putcUart0('\n');
    putsUart0("R12\t= ");
    printHex((uint32_t)*(PSPAdd+4));
    putcUart0('\n');
    putsUart0("LR\t= ");
    printHex((uint32_t)*(PSPAdd+5));
    putcUart0('\n');
    putsUart0("PC\t= ");
    printHex((uint32_t)*(PSPAdd+6));
    putcUart0('\n');
    putsUart0("xPSR= ");
    printHex((uint32_t)*(PSPAdd+7));

    putsUart0("\n\n");

    clearMPUFaultBit(); //p.175

    //trigger pendSV ISR
    triggerPENDSVIsrCall();
}

// REQUIRED: code this function
void hardFaultIsr(){
    uint32_t *hardFaultFlags = 0xE000ED2C;
    uint32_t *PSPAdd = getPSP();
    uint32_t *MSPAdd = getMSP();
    putsUart0("Hard fault in process N\n");
    putsUart0("PSP: ");
//    printHex((uint32_t)PSPAdd);
//    putsUart0(" : ");
    printHex((uint32_t)*PSPAdd);
    putsUart0("\n");
    putsUart0("MSP: ");
//    printHex((uint32_t)MSPAdd);
//    putsUart0(" : ");
    printHex((uint32_t)*MSPAdd);
    putsUart0("\n");
    putsUart0("Hard fault flags: ");
    printHex((uint32_t)*hardFaultFlags);
//    putsUart0(" : ");
//    printHex((uint32_t)*hardFaultFlags);
    putcUart0('\n');
    putcUart0('\n');
    while(1);
}

// REQUIRED: code this function
void busFaultIsr(){
    putsUart0("Bus fault in process N\n");
}

// REQUIRED: code this function
void usageFaultIsr(){
    putsUart0("Usage fault in process N\n");
}

//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

// Initialize Hardware
// REQUIRED: Add initialization for blue, orange, red, green, and yellow LEDs
//           6 pushbuttons
void initHw() {
    initSystemClockTo40Mhz();
    enablePort(PORTA);
    enablePort(PORTC);
    enablePort(PORTD);
    enablePort(PORTE);
    enablePort(PORTF);

    //LEDs
    selectPinPushPullOutput(PORTA,2); //Red
    selectPinPushPullOutput(PORTA,3); //Orange
    selectPinPushPullOutput(PORTA,4); //Yellow

    selectPinPushPullOutput(PORTE,0); //green

    selectPinPushPullOutput(PORTF,2); //blue


    //Push Buttons
    selectPinDigitalInput(PORTC, 4);    //PB5   0010 0000
    selectPinDigitalInput(PORTC, 5);    //PB4   0001 0000
    selectPinDigitalInput(PORTC, 6);    //PB3   0000 1000
    selectPinDigitalInput(PORTC, 7);    //PB2   0000 0100

    //unlock PortD
    setPinCommitControl(PORTD, 7);

    selectPinDigitalInput(PORTD, 6);    //PB1   0000 0010
    selectPinDigitalInput(PORTD, 7);    //PB0   0000 0001

    enablePinPullup(PORTC, 4);
    enablePinPullup(PORTC, 5);
    enablePinPullup(PORTC, 6);
    enablePinPullup(PORTC, 7);
    enablePinPullup(PORTD, 6);
    enablePinPullup(PORTD, 7);
}

// REQUIRED: add code to return a value from 0-63 indicating which of 6 PBs are pressed
uint8_t readPbs() {
    uint8_t sum = 0;

    if(getPinValue(PORTD, 7) == 0)
        sum += 1;
    if(getPinValue(PORTD, 6) == 0)
        sum += 2;
    if(getPinValue(PORTC, 7) == 0)
        sum += 4;
    if(getPinValue(PORTC, 6) == 0)
        sum += 8;
    if(getPinValue(PORTC, 5) == 0)
        sum += 16;
    if(getPinValue(PORTC, 4) == 0)
        sum += 32;
    return sum;
}

//-----------------------------------------------------------------------------
// YOUR UNIQUE CODE
// REQUIRED: add any custom code in this space
//-----------------------------------------------------------------------------


//timer
void initTimer1() {
    SYSCTL_RCGCTIMER_R |= SYSCTL_RCGCTIMER_R1;
    _delay_cycles(3);

    // Configure Timer 1 as the time base
    TIMER1_CTL_R &= ~TIMER_CTL_TAEN;                 // turn-off timer before reconfiguring
    TIMER1_CFG_R = TIMER_CFG_32_BIT_TIMER;           // configure as 32-bit timer (A+B)
    TIMER1_TAMR_R = TIMER_TAMR_TAMR_PERIOD | TIMER_TAMR_TACDIR; // configure for edge count mode, count up
    TIMER1_IMR_R = 0;                                // turn-on interrupts
}


void getIpcsData(semaphore *semphorePtr, uint32_t *count) {
    __asm(" SVC #16");
}
void getPsData(processData *psData, uint32_t *count) {
    __asm(" SVC #17");
}
//Shell call functions
//--------------------------------

void reboot() {
    __asm(" SVC #12");
}
void ps() {
    uint32_t count = 0;
    processData psData[MAX_TASKS];
    getPsData(psData, &count);
    uint8_t i = 0;
    putsUart0("Pid\t\t| Name\t\t| State\t\t| Time %\n");
    for(; i < count; i++) {
        printTwoBytesHex(psData[i].pid);
        putsUart0("\t");
        putsUart0(psData[i].name);
        if(stringsEqual(psData[i].name, "Shell") || stringsEqual(psData[i].name, "Idle")) {
            putsUart0("\t");
        }
        if(!stringsEqual(psData[i].name, "LengthyFn")) {
            putsUart0("\t");
        }
        putsUart0("\t");
        printState(psData[i].state);
        if(psData[i].state < 4) {
            putsUart0("\t");
        }
        putsUart0("\t");
//        printHex(psData[i].time);
        printNumberPercentage(psData[i].time);
//        putsUart0("\t");
//        printNumberPercentage(totalProcessTime);
        putsUart0("\n");
    }
}
//THIS WILL CAUSE AN MPU FAULT. YOU CANNOT READ TCB IN THREAD MODE.
    //SOLUTION: create a new struct of semaphores that includes a name, that way you will not have to look through the tcb to find the name.
void ipcs() {
//    putsUart0("ipcs called\n");
    semaphoreData semaphoreArr[MAX_SEMAPHORES];
    uint32_t count = 0;
    getIpcsData(semaphoreArr, &count);
    uint8_t i = 0;
    putsUart0("Semaphore\t| Count\t|Queue\n");
    for(i = 0; i < count; i++) {
        if(i == 0){
            putsUart0("keyPressed\t");
        } else if(i == 1) {
            putsUart0("keyReleased\t");
        } else if(i == 2) {
            putsUart0("flashReq\t\t");
        } else {
            putsUart0("resource\t\t");
        }
        putsUart0("     ");
        uint8_t count = semaphoreArr[i].count;
        uint8_t queueSize = semaphoreArr[i].queueSize;
        char *countString = intToString(count);
        putsUart0(countString);
//        printTwoBytesHex(count);
        putsUart0("\t\t");
        uint8_t j = 0;
        for(j = 0; j < queueSize; j++) {
            uint8_t index = semaphoreArr[i].processQueue[j];
            putsUart0(semaphoreArr[i].processNames[j]);
            putsUart0(" ");
        }
        putsUart0("\n");
    }
}
void killPid(uint32_t pid) {
    __asm(" SVC #14");
}
//void pi(bool on) {
//    if(on)
//        putsUart0("pi ON\n");
//    else
//        putsUart0("pi OFF\n");
//}
void preempt(uint8_t on) {
    __asm(" SVC #11");
}
void sched(uint8_t prio_on) {
    __asm(" SVC #10");
}
void getPidof(uint32_t *pid, char *name) {
    __asm(" SVC #13");
}
void pidof(char name[]) {
    uint32_t *pid;
    getPidof(pid, name);
    if(*pid != 0xFFFFFFFF) {
        printTwoBytesHex((uint16_t)*pid);
        putsUart0("\n");
    }
    else {
        putsUart0("no matching process found\n");
    }
}
void proc_name(char *s) {
    __asm(" SVC #15");
}

// ------------------------------------------------------------------------------
//  Task functions
// ------------------------------------------------------------------------------

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
void idle()
{
    while(true)
    {
//        __asm(" MOV R0, #0x7");
//        __asm(" MOV R1, #0x8");
//        __asm(" MOV R2, #0x9");
//        __asm(" MOV R3, #0x10");
//        __asm(" MOV R4, #0x11");
//        __asm(" MOV R5, #0x12");
//        __asm(" MOV R6, #0x13");
//        __asm(" MOV R7, #0x14");
//        __asm(" MOV R8, #0x15");
//        __asm(" MOV R9, #0x16");
//        __asm(" MOV R10, #0x17");
//        __asm(" MOV R11, #0x18");

        ORANGE_LED = 1;
        waitMicrosecond(1000);
        ORANGE_LED = 0;
        yield();
    }
}

void flash4Hz()
{
    while(true)
    {
        GREEN_LED ^= 1;
        sleep(125);
    }
}

void oneshot()
{
    while(true)
    {
        wait(flashReq);
        YELLOW_LED = 1;
        sleep(1000);
        YELLOW_LED = 0;
    }
}

// one task must be ready at all times or the scheduler will fail
// the idle task is implemented for this purpose
//run idle2 and idle1 for STEP7
void idle2()
{
    while(true)
    {
        RED_LED = 1;
        waitMicrosecond(1000);
        RED_LED = 0;
        yield();
    }
}
void partOfLengthyFn()
{
    // represent some lengthy operation
    waitMicrosecond(990);
    // give another process a chance to run
    yield();
}

void lengthyFn()
{
    uint16_t i;
    while(true)
    {
        wait(resource);
        for (i = 0; i < 5000; i++)
        {
            partOfLengthyFn();
        }
        RED_LED ^= 1;
        post(resource);
    }
}

void readKeys()
{
    uint8_t buttons;
    while(true)
    {
        wait(keyReleased);
        buttons = 0;
        while (buttons == 0)
        {
            buttons = readPbs();
            yield();
        }
        post(keyPressed);
        if ((buttons & 1) != 0)
        {
            YELLOW_LED ^= 1;
            RED_LED = 1;
        }
        if ((buttons & 2) != 0)
        {
            post(flashReq);
            RED_LED = 0;
        }
        if ((buttons & 4) != 0)
        {
            restartThread(flash4Hz);
        }
        if ((buttons & 8) != 0)
        {
            destroyThread(flash4Hz);
        }
        if ((buttons & 16) != 0)
        {
            setThreadPriority(lengthyFn, 4);
        }
        yield();
    }
}

void debounce()
{
    uint8_t count;
    while(true)
    {
        wait(keyPressed);
        count = 10;
        while (count != 0)
        {
            sleep(10);
            if (readPbs() == 0)
                count--;
            else
                count = 10;
        }
        post(keyReleased);
    }
}

void uncooperative()
{
    while(true)
    {
        while (readPbs() == 8)
        {
        }
        yield();
    }
}

void errant()
{
    uint32_t* p = (uint32_t*)0x20000020;
    while(true)
    {
        while (readPbs() == 32)
//        while(1)
        {
            *p = 0;
        }
        yield();
    }
}

void important()
{
    while(true)
    {
        wait(resource);
        BLUE_LED = 1;
        sleep(1000);
        BLUE_LED = 0;
        post(resource);
    }
}

// REQUIRED: add processing for the shell commands through the UART here
void shell()
{
    shellFunction();
}

//-----------------------------------------------------------------------------
// Main
//-----------------------------------------------------------------------------

int main(void)
{
    bool ok;

    // Initialize hardware
    initHw();
    initTimer1();
    initUart0();
    initRtos();

    // Setup UART0 baud rate
    setUart0BaudRate(115200, 40e6);

    // Power-up flash
    GREEN_LED = 1;
    waitMicrosecond(250000);
    GREEN_LED = 0;
    waitMicrosecond(250000);

    // Initialize semaphores
    createSemaphore(keyPressed, 1);
    createSemaphore(keyReleased, 0);
    createSemaphore(flashReq, 5);
    createSemaphore(resource, 1);

    // Add required idle process at lowest priority
    ok =  createThread(idle, "Idle", 7, 1024); //idle2
//    ok =  createThread(idle2, "Idle2", 7, 1024); //idle2

    // Add other processes
    ok &= createThread(lengthyFn, "LengthyFn", 6, 1024);
    ok &= createThread(flash4Hz, "Flash4Hz", 4, 1024);
    ok &= createThread(oneshot, "OneShot", 2, 1024);
    ok &= createThread(readKeys, "ReadKeys", 6, 1024);
    ok &= createThread(debounce, "Debounce", 6, 1024);
    ok &= createThread(important, "Important", 0, 1024);
    ok &= createThread(uncooperative, "Uncoop", 6, 1024);
    ok &= createThread(errant, "Errant", 6, 2048);
    ok &= createThread(shell, "Shell", 6, 2048);

    // Start up RTOS
    if (ok)
        startRtos(); // never returns
    else
        RED_LED = 1;

    return 0;
}
