#pragma once
#include "Constants.h"

/* #################### DEBUG PANEL #################### */
// #define debug_i2cTransmitTime 
// #define debug_fastPrints //isr indicator and BLOCK#
// #define debug_hyperFastPrints
// #define debug_hyperFastPrintsWithPot //toggles on Blok Period printing
// #define debug_useTagFlag
// #define DEBUG_ALLOW_DUMPING
 
/* #################### USER SET-SETTINGS #################### */
// #define useGPTimerOverESP32Timer
#define lastResort
#define startingDuty (0.85) //, normally .8
// #define as5600DirPinHigh
// #define as5600DirPinHighAtCalibration


/* #################### RUNTIME VARIABLES #################### */
/* ========================= C++ STRUCTS ========================= */
typedef enum {
    POSITION_CONTROL,
    VELOCITY_CONTROL,
    TORQUE_CONTROL
} control_type;

typedef struct{
    int oldSectorTarget = 0;
    int sectorTarget = 0; //for stator current vector
    std::atomic<uint32_t> blockPeriod = 10000.0; //6941
    std::atomic<uint32_t> tlog_readAS5600 = 0;
    std::atomic<bool> setMotorFreeSpin = false;
    std::atomic<bool> setMotorFreeTemporarily = false;
    int dir = 5; // 5=cw (-), 2 for ccw(+) (2 for half working AS5600)
    control_type controlMethod = VELOCITY_CONTROL;
    /*PID variables*/
    int rotorVal =0; //needs to inversted
    float targetPosition =0; //target Position in bits
    float targetVelocity =0; //target RPS
    float targetAcceleration =0; //target RPS
    
    uint32_t measuredPos[cBufSize]; //recent values at the front
    float measuredVel[cBufSize]; //bits/s
    float measuredAccel[cBufSize];
    std::atomic <uint32_t> pindex= 0;
    std::atomic <uint32_t> vindex= 0;
    std::atomic <uint32_t> aindex= 0;
    float lastPosError = 0; //for dx/dt
    float totalPosChange = 0; //∫v(t)dt
    
    //Velocity pid
    float lastVelError = 0; //dv/dt
    float totalVelChange = 0; //∫a(t)dt, area
} gVar_t;

/* ========================= GLOBAL VARIABLES  ========================= */
inline DRAM_ATTR volatile std::atomic<uint32_t> isr2i =0;
volatile DRAM_ATTR inline gVar_t global;
inline portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;

/* ------------------------------ DEBUG-TOGGLED VARIABLES  ------------------------------ */
#ifdef debug_hyperFastPrints
volatile inline DRAM_ATTR const char* darray[10000];
volatile inline DRAM_ATTR std::atomic<uint32_t> dindex []={0,0}; //new, old
#endif
#if (defined(debug_hyperFastPrints) || defined(debug_fastPrints))
DRAM_ATTR constexpr const char* ghgl[6] = {"0BA ","1CA ","2CB ","3AB ","4AC ","5BC "}; //[-30,30) = block 0
DRAM_ATTR constexpr const char* dgdir[6] = {"∅","D?","+","D?","NOT-","-"};
#endif
/* ------------------------------ HANDLES  ------------------------------ */
extern adc_oneshot_unit_handle_t adcHandle;
extern TaskHandle_t initializeI2CTask;

extern  intr_handle_t oneBlockISR;
inline mcpwm_timer_handle_t VTimer =NULL;

inline TaskHandle_t setupTask= NULL;
inline DRAM_ATTR TaskHandle_t getSectorNumberTask= NULL;
inline TaskHandle_t mathItOutTask= NULL;
inline DRAM_ATTR TaskHandle_t executeGatesTask= NULL;


/* #################### FUNCTION DECLARATIONS #################### */
void readPotRepeat(void * parameter);
uint32_t readPotOnce(bool filter, int averager);
void spamSearchCV(void *parameter);
void initialize(void *parameter);      
void tag(const char* tag);
void tagFlag(bool start, int timer);
void d_blockCycling(void * startTick5);


/*#################### BACKEND #################### */
/* ========================= AS5600 SENSOR CALIBRATION  ========================= */
//top view of physical motor has ABC going ccw, [-30 degrees, 30 degrees) = block 0
//((4096-global.rotorVal)+(int)((4096.0)*(38.0/36.0) - (4096-(3388)) )) ==> (4096/18+3388-val)*18/4096==>>(7711.5-v)*0.00439453
#ifdef as5600DirPinHighAtCalibration
#define as5600CalibratedOffset (int)((4096.0) * (38.0 / 36.0) - (as5600CalibrationRawValue) )  
#else
#define as5600CalibratedOffset (int)((4096.0) * (38.0 / 36.0) - (4096 - as5600CalibrationRawValue) )  
#endif

#ifdef as5600DirPinHigh //When motor is running controller code
#define getRotorValAdjusted(x) (as5600CalibratedOffset + x) * SECTOR_PER_BITS
#else
#define getRotorValAdjusted(x) ((4096 - x) + as5600CalibratedOffset) * SECTOR_PER_BITS
#endif
/* ========================= MOTOR LIMITS SHORTHAND CHECK  ========================= */
// #define debug_defCheck1      /* motor spec, float constants */
// #define debug_defCheck2      /* software limit settings, float constants */
// #define debug_defCheck3      /* target bounds, float constants */
// #define debug_defCheck4      /* software period ticks limit, uint32_T constants */

static_assert( ( MOTOR_SPEC_MIN_VELOCITY <= SL_MIN_VELOCITY) &&  ( SL_MIN_VELOCITY < SL_MAX_VELOCITY ) && (SL_MAX_VELOCITY <= MOTOR_SPEC_MAX_VELOCITY) );
static_assert( ( MOTOR_SPEC_MIN_TORQUE <= SL_MIN_TORQUE) && ( SL_MIN_TORQUE < SL_MAX_TORQUE ) && (SL_MAX_TORQUE <= MOTOR_SPEC_MAX_TORQUE) );

static_assert( ( -SL_MAX_VELOCITY <= TARGET_VELOCITY_LB) && ( TARGET_VELOCITY_LB < TARGET_VELOCITY_UB ) && (TARGET_VELOCITY_UB <= SL_MAX_VELOCITY) );
static_assert( ( -SL_MAX_TORQUE <= TARGET_TORQUE_LB) && ( TARGET_TORQUE_LB < TARGET_TORQUE_UB ) && (TARGET_TORQUE_UB <= SL_MAX_TORQUE) );

#ifdef debug_defCheck1
static_assert(MOTOR_SPEC_MAX_VELOCITY >= 0xFFFFFFFE);
static_assert(MOTOR_SPEC_MIN_VELOCITY >= 0xFFFFFFFE);
static_assert(MOTOR_SPEC_MAX_TORQUE >= 0xFFFFFFFE);
static_assert(MOTOR_SPEC_MIN_TORQUE >= 0xFFFFFFFE);
#endif
#ifdef debug_defCheck2
static_assert(SL_MAX_VELOCITY >= 0xFFFFFFFE);
static_assert(SL_MIN_VELOCITY >= 0xFFFFFFFE);
static_assert(SL_MAX_TORQUE >= 0xFFFFFFFE);
static_assert(SL_MIN_TORQUE >= 0xFFFFFFFE);
#endif
#ifdef debug_defCheck3
static_assert(TARGET_POSITION_UB >= 0xFFFFFFFE);
static_assert(TARGET_POSITION_LB >= 0xFFFFFFFE);
static_assert(TARGET_VELOCITY_UB >= 0xFFFFFFFE);
static_assert(TARGET_VELOCITY_LB >= 0xFFFFFFFE);
static_assert(TARGET_TORQUE_UB >= 0xFFFFFFFE);
static_assert(TARGET_TORQUE_LB >= 0xFFFFFFFE);
#endif
#ifdef debug_defCheck4
static_assert(SL_MAX_VELOCITY_PERIOD_TICKS >= 0xFFFFFFFE);
static_assert(SL_MIN_VELOCITY_PERIOD_TICKS >= 0xFFFFFFFE);
static_assert(VTICKS_PER_BLOCK >= 0xFFFFFFFE);
// static_assert(LMAP(3, 4, 3, 0, 0) >= 0xFFFFFFFE);
// static_assert(LMAP(3., 4, 3, 0, 0) >= 0xFFFFFFFE);
// static_assert(LMAP(3., 4, 3, 0.0, 0) >= 0xFFFFFFFE);
// static_assert(LMAP(3, 4, 4, 0, 1) >= 0xFFFFFFFE);
// static_assert(LMAP(3., 4, 4, 0, 1) >= 0xFFFFFFFE);
#endif
