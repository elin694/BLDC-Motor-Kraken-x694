#pragma once
#include "Constants.h"
/*=============================DEBUG CONTROL PANEL=============================*/
#define lastResort
#define debug_printRPS 
// #define debug_fastPrints //isr indicator and BLOCK#
#define debug_i2cTransmitTime 
// #define debug_hyperFastPrints
#define debug_hyperFastPrintsWithPot //toggles on Blok Period printing
// #define debug_useTagFlag

#define startingDuty (0.8) //, normally .8
#define cBufSize 8
#define velPotReadPeriod (int)(20) //set velocity via pot 1
#define initializationLatency pdMS_TO_TICKS(30)
/*=============================USER SETTING CONTROL PANEL=============================*/
//.03 ->56 in 612 =.0915
//.6-->189 in 114s = 1.658
//.9 --> 292 in 192 = 1.52  

#define estimatedI2CReadTime_us (uint32_t)(200) //694
#define i2cClockSpeed 1000000
#define i2cWaitout 1 //in ms
#define mcpwm_lowSideGroupPrescaler 40
#define HighTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler)) //125ns , must not simple ratio
#define VTimerResolution  (uint32_t)(16e7/(mcpwm_lowSideGroupPrescaler*10)) //125ns , must not simple ratio

/*minimum and maximum RPS */
#define maxf_HTimerPeriod (1111/2) //200--> 111.11rps, 1111-->20rps
#define minf_HTimerPeriod (uint32_t)(65535/2)
// #define fMin (float)(VTimerResolution/(18.0f*minf_HTimerPeriod))
#define fMin (float)(VTimerResolution/(18.0f*-maxf_HTimerPeriod))
#define fMax (float)(VTimerResolution/(18.0f*maxf_HTimerPeriod)) 

#define aMin (float)(VTimerResolution/(18.0f*-maxf_HTimerPeriod))
#define aMax (float)(VTimerResolution/(18.0f*maxf_HTimerPeriod)) 

#define pMin (float)(0)
#define pMax (float)(3*3.141592653/2)


inline DRAM_ATTR volatile std::atomic<uint32_t> isr2i =0;
//============================= INTERRUPT PRIORITY=============================
#define MCPWM_HighsideIntrPriority 1 //tep,tez
#define MCPWM_LowsideIntrPriority 1 //tep,tez
#define runOnMCPWMIntrPriority ESP_INTR_FLAG_LEVEL2 //might be a bit long
#define i2c_intrPriority 3
/*esp timer intr : 1-3, (2)
freertos timer :lvl 1 or 3 (1)
watchdog and sys checks :4 or 5 */
//++++++++++++++++++++++++++++++MCPWM++++++++++++++++++++++++++++++
#define activePwmPeriod (uint32_t)(HighTimerResolution/20000)  //change to 20khz when high
#define startingGateCmpValue (uint32_t)((1-startingDuty)*activePwmPeriod/2.0) //High gate comparator's comparatorValue when ON; can be modified later
#define maxDuty 0.95f
#define minDuty 0.03f
// #if ((startingDuty < minDuty) || (startingDuty > maxDuty))
// #warning "DUTY out of bounds!!!!!!!!!!!!!!!!!!!!!!!!!"
// #endif

#define maxRPS 30
#define minRPS 1

//+++++++++++++++++++++++++++++++++++RUNTIME VARIABLES+++++++++++++++++++++++++++++++++++
typedef enum {
    POSITION_CONTROL,
    VELOCITY_CONTROL,
    TORQUE_CONTROL
} control_type;

typedef struct{
    const char* array[100000];
    std::atomic<uint32_t> i=0; //new, old
}debugStruct;

typedef struct{
    int oldSectorTarget = 0;
    int sectorTarget = 0; //for stator current vector
    std::atomic<uint32_t> blockPeriod = 10000.0; //6941
    std::atomic<uint32_t> tlog_readAS5600 = 0;
    std::atomic<bool> newVelPotValue = false;
    std::atomic<bool> setMotorFreeSpin = false;
    std::atomic<bool> setMotorFreeTemporarily = false;
    
    control_type controlMethod = VELOCITY_CONTROL;
    /*PID variables*/
    int rotorVal =0; //needs to inversted
    float targetPosition =0; //target Position in bits
    float targetVelocity =0; //target RPS
    float targetAcceleration =0; //target RPS

    uint32_t measuredPos[cBufSize]; //recent values at the front
    float measuredVel[cBufSize];
    float measuredAccel[cBufSize];
    std::atomic <uint32_t> pindex= 0;
    std::atomic <uint32_t> vindex= 0;
    std::atomic <uint32_t> aindex= 0;
    float lastPosError = 0; //dx/dt
    float totalPosChange = 0; //∫v(t)dt

    //Velocity pid
    float lastVelError = 0; //dv/dt
    float totalVelChange = 0; //∫a(t)dt, area
    int dir = 5; // 5=cw (-), 2 for ccw(+) (2 for half working AS5600)
} gVar_t;
volatile DRAM_ATTR inline gVar_t global;
inline portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;

//================================Handles================================
extern adc_oneshot_unit_handle_t adcHandle;
extern TaskHandle_t initializeI2CTask;

extern  intr_handle_t oneBlockISR;
inline mcpwm_timer_handle_t VTimer =NULL;

inline TaskHandle_t setupTask= NULL;
inline TaskHandle_t getSectorNumberTask= NULL;
inline TaskHandle_t mathItOutTask= NULL;
inline TaskHandle_t executeGatesTask= NULL;
//====================FUNCTION DECLARATION =======================
void readPotRepeat(void * parameter);
uint32_t readPotOnce(bool filter, int averager);
void getTimerCountNow(const char* str);
void spamSearchCV(void *parameter);
void initialize(void *parameter);      
void tag(const char* tag);
void tagFlag(bool start, int timer);
void d_blockCycling(void * startTick5);

#define ExecuteGate__FreeSpin_NotifVal 0x0000FFFF
//
#ifdef debug_hyperFastPrints
volatile inline DRAM_ATTR const char* darray[10000];
volatile inline DRAM_ATTR std::atomic<uint32_t> dindex []={0,0}; //new, old
#endif