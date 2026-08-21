#pragma once
#include "Constants.h"

/* #################### DEBUG PANEL #################### */
// #define debug_i2cTransmitTime 
// #define debug_fastPrints //isr indicator and BLOCK#
// #define debug_hyperFastPrints
// #define debug_hyperFastPrintsWithPot //toggles on Blok Period printing
// #define debug_useTagFlag
// #define DEBUG_ALLOW_DUMPING
// #define DEBUG_ALLOW_ONE_TIME_DUMPING
 
/* #################### USER SET-SETTINGS #################### */
/*Enable PID Modes
not defined- open, feed forward loop (if it stalls it stalls)
def TORQUE_CONTROL - only test Torque control loop in action
def VELOCITY_CONTROL - only test Torque and velocity control loop in action
def POSITION_CONTROL - only test Torque, velocity, and Postion control loop in action
*/
#define ALLOWED_LOOPS_TO_TEST TORQUE_CONTROL

// #define useGPTimerOverESP32Timer
#define lastResort
// #define ENABLE_GAMBLING_ON_I2C
#define startingDuty (0.6) //, normally .8
// #define as5600DirPinHigh
// #define as5600DirPinHighAtCalibration


/* #################### RUNTIME VARIABLES #################### */
/* ========================= C++ STRUCTS ========================= */
typedef struct {
    mcpwm_timer_config_t timerConfig;
    mcpwm_operator_config_t opConfig;
    mcpwm_comparator_config_t compConfig;
    mcpwm_generator_config_t pwmConfig;

    mcpwm_timer_handle_t timer = NULL;
    mcpwm_oper_handle_t operatorModule= NULL;
    mcpwm_cmpr_handle_t comparator0 = NULL;
    mcpwm_cmpr_handle_t comparator1 = NULL; //null for high
    mcpwm_gen_handle_t pwmGate0 = NULL;
    mcpwm_gen_handle_t pwmGate1 = NULL;// stays null
    //shoutout gemini for suggest changing countval
} phaseMcpwm;

typedef struct{
    int oldSectorTarget = 0;
    int sectorTarget = 0; //for stator current vector
    std::atomic<uint32_t> blockPeriod = 10000.0; //6941
    std::atomic<uint32_t> tlog_readAS5600;
    std::atomic<uint32_t> tlog_trailingReadAS5600 =0;
    std::atomic<bool> setMotorFreeSpin = false;
    std::atomic<bool> setMotorFreeTemporarily = false;
    int dir = 5; // 5=cw (-), 2 for ccw(+) (2 for half working AS5600)
    #ifndef ALLOWED_LOOPS_TO_TEST 
    #define controlk VELOCITY_CONTROL
    #else 
    #define controlk ALLOWED_LOOPS_TO_TEST
    #endif
    control_type controlMethod = controlk;
    /*PID variables*/
    int rotorVal =0; //needs to inversted
    float targetTorque =0; //target RPS
    float targetVelocity =0; //target RPS
    int targetPosition_BiPS =0; //target Position in bits

    // volatile uint32_t * pTorquePID = NULL;
    // volatile uint32_t * pVelocityPID = NULL;
    // volatile uint32_t * pRositionPID = NULL;
} gVar_t;

/* ========================= GLOBAL VARIABLES  ========================= */
inline DRAM_ATTR volatile std::atomic<uint32_t> isr2i =0;
volatile DRAM_ATTR inline gVar_t global;
// gpio 19- miso, b High side is tx2
extern phaseMcpwm motorH[3];
extern phaseMcpwm motorL[3];
inline DRAM_ATTR portMUX_TYPE stepPeriodMux = portMUX_INITIALIZER_UNLOCKED;
inline DRAM_ATTR portMUX_TYPE sensorMux = portMUX_INITIALIZER_UNLOCKED;

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
inline DRAM_ATTR TaskHandle_t positionControlLoopTask = NULL;
inline DRAM_ATTR TaskHandle_t velocityControlLoopTask = NULL;
inline DRAM_ATTR TaskHandle_t torqueControlLoopTask = NULL;
inline DRAM_ATTR TaskHandle_t stableLoopCheckTask = NULL;

inline DRAM_ATTR TaskHandle_t getSectorNumberTask= NULL;
inline DRAM_ATTR TaskHandle_t executeGatesTask= NULL;


/* #################### FUNCTION DECLARATIONS #################### */
void readPotRepeat(void * parameter);
uint32_t readPotOnce(bool filter, int averager);
void spamSearchCV(void *parameter);
void initialize(void *parameter);      
void tag(const char* tag);
void tagFlag(bool start, int timer);


/*#################### BACKEND #################### */
/* ========================= AS5600 SENSOR CALIBRATION  ========================= */
//top view of physical motor has ABC going ccw, [-30 degrees, 30 degrees) = block 0
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

/*Check software limits are within hardware specs*/
static_assert( ( MOTOR_SPEC_MIN_VELOCITY <= SL_MIN_VELOCITY) &&  ( SL_MIN_VELOCITY < SL_MAX_VELOCITY ) && (SL_MAX_VELOCITY <= MOTOR_SPEC_MAX_VELOCITY) );
static_assert( ( MOTOR_SPEC_MIN_TORQUE <= SL_MIN_TORQUE) && ( SL_MIN_TORQUE < SL_MAX_TORQUE ) && (SL_MAX_TORQUE <= MOTOR_SPEC_MAX_TORQUE) );
static_assert( 0.0f < minDuty &&  minDuty < maxDuty && maxDuty < 1.0f );

/*Check target Bounds are within software limits*/
static_assert( ( -SL_MAX_VELOCITY <= TARGET_VELOCITY_LB) && ( TARGET_VELOCITY_LB < TARGET_VELOCITY_UB ) && (TARGET_VELOCITY_UB <= SL_MAX_VELOCITY) );
static_assert( ( -SL_MAX_TORQUE <= TARGET_TORQUE_LB) && ( TARGET_TORQUE_LB < TARGET_TORQUE_UB ) && (TARGET_TORQUE_UB <= SL_MAX_TORQUE) );

#define AS5600_POLL_FREQ_HZ (1e6*1.0/estimatedI2CReadTime_us)
/* Since calculation is done in  torque control loop, data must come in faster than  torque control period*/
static_assert(AS5600_POLL_FREQ_HZ -100 >= TORQUE_CL_FREQ_HZ);

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
