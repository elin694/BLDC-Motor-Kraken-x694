#include "Globals.h"
#include "Controller.h"
//Backend Functions, ASSES/ASSERT TARGETS IN FRONTEND
void startAllTimersAndInterrupts(void * startTick6){ 
   TickType_t startTick = *(TickType_t*)startTick6;
   ESP_LOGI(blue "controller.cpp\n", "#######  STARTING TIMERS ####### ");
   xTaskDelayUntil(&startTick,initializationLatency);
   ESP_ERROR_CHECK(mcpwm_timer_start_stop(VTimer, MCPWM_TIMER_START_NO_STOP));
   
   #ifdef ALLOWED_LOOPS_TO_TEST
   #if (ALLOWED_LOOPS_TO_TEST >= TORQUE_CONTROL)
   ESP_ERROR_CHECK( gptimer_start(torqueLoop.timer) );
   #if (ALLOWED_LOOPS_TO_TEST >=VELOCITY_CONTROL)
   ESP_ERROR_CHECK( gptimer_start(velocityLoop.timer) );
   #if (ALLOWED_LOOPS_TO_TEST >= POSITION_CONTROL)
   ESP_ERROR_CHECK( gptimer_start(positionLoop.timer) );
   #endif
   #endif
   #endif
   #endif

   #ifndef lastResort
   mcpwm_int_clr_reg_t clearReg = {.val = ~((uint32_t)(0x00000000))};
   MCPWMx->int_clr.val=  clearReg.val;
   MCPWMx->int_ena.timer0_tez_int_ena = 1; 
   #endif
   vTaskDelete(NULL);
}

/*Keeps <STABLE_SLOTS> for past history*/
#define STABLE_SLOTS 4 
/* WRITES ONLY TO x.overIntegration FLAGS*/
void stableLoopCheck(void * startTick7){ //updates arrrays with new ifo
    CLEAR_ALL_NOTIFS(NULL);
    TickType_t startTick = *(TickType_t*) startTick7;
    uint32_t ix = 0;
    int mPos[STABLE_SLOTS];
    float mVel[STABLE_SLOTS], mAccel[STABLE_SLOTS], localTime[STABLE_SLOTS];
    TickType_t loopStartTick;
    volatile gVar_t* gv = &global;

   xTaskDelayUntil(&startTick, initializationLatency);
   for(;;){
        loopStartTick = xTaskGetTickCount();

        //slow loop so it checks the general pid trendx 
        vTaskDelayUntil( &loopStartTick, pdMS_TO_TICKS(10));
        const uint32_t idxNow = ix % STABLE_SLOTS;
        const uint32_t lastIdx = (ix - 1) % STABLE_SLOTS;

        int encoder = global.rotorVal;
        localTime[idxNow] = gv-> tlog_readAS5600;
        float dt_s = (localTime[idxNow] - localTime[lastIdx]) / 1.0e6;
        mPos[idxNow] = encoder;
        mVel[idxNow] = (mPos[idxNow] - mPos[lastIdx]) * BITS_TO_ROTATIONS / (dt_s);
        mAccel[idxNow] = (mVel[idxNow] - mVel[lastIdx]) / (dt_s);
        if ( fabsf(mVel[idxNow]) <= STABLE_VELOCITY_THRESHOLD) {
            positionLoop.overIntegration = true;
        } else {
            positionLoop.overIntegration = false;
        }

        if ( fabsf(mAccel[idxNow]) <= STABLE_ACCEL_THRESHOLD){
            velocityLoop.overIntegration = true;
        } else {
            velocityLoop.overIntegration = false;
        }
        ix++;
        taskYIELD();
   }
}

#define CL_NOTIF_INDEX 1
/* WRITES ONLY TO x. mindex, x. measured[] ARRAYS AND global. dir */
void IRAM_ATTR torqueControlLoop(void* pointerToTarget) { /* WRITES ALL INDEXES*/
    CLEAR_ALL_NOTIFS(NULL);

    /*Filling in the scaffold form*/
    #define TORQUE_CL_COUNT (CL_TIMER_FREQ_HZ / TORQUE_CL_FREQ_HZ)
    torqueLoop.timerConfig.intr_priority = TORQUE_LOOP_TIMER_INTR_PRIORITY;
    torqueLoop.alarmConfig.alarm_count =TORQUE_CL_COUNT;
    torqueLoop.callbackEvent.on_alarm = torqueCtrlCBK;
    
    /*Submitting the form*/
    ESP_ERROR_CHECK(gptimer_new_timer(&torqueLoop.timerConfig, &torqueLoop.timer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(torqueLoop.timer, &torqueLoop.alarmConfig));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(torqueLoop.timer, &torqueLoop.callbackEvent, &torqueLoop));
    ESP_ERROR_CHECK(gptimer_enable(torqueLoop.timer));
    ESP_ERROR_CHECK(gptimer_get_resolution(torqueLoop.timer, (uint32_t*) &torqueLoop.freq));
    ESP_LOGI("tqLoop", "Frequency: %d", torqueLoop.freq / TORQUE_CL_COUNT);
    int tlog_lastRecordedInteration = SNAP();

    /* Local Global Cosntants to use in this Calculation/ Torque Loop
    - also consider case from motor stall - to SL_MIN_VELOCITY
    */
    gVar_t* pGlobalVar = (gVar_t*) pointerToTarget;
    float* pTargetTorque = &((pGlobalVar) -> targetTorque);

    for(;;){
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
        
        int tlog_now = SNAP();
        taskENTER_CRITICAL( &sensorMux );
        // uint32_t dt  = pGlobalVar-> tlog_readAS5600 - pGlobalVar-> tlog_trailingReadAS5600;
        uint32_t tLastRead  = pGlobalVar-> tlog_readAS5600;
        taskEXIT_CRITICAL( &sensorMux );
        
        /*Checking the last time it read ad5600 to ensure it uses updated/recent data
            If it is recent, proceed with torque/calc, and send the go notify of the update
            Else do nothing (motor freeSpins).

            IF the iwndow is larger than i2c read period, then there is 1 slo buffer for errors.
        */
        if( tlog_now - tLastRead < ACCEPTABLE_I2C_READ_WINDOW ){
            /*Changes Kinamatic Values via shift and set */
            // int localTime = pGlobalVar-> tlog_readAS5600;
            uint32_t pLIdx = (positionLoop.mindex++) % CL_CIRCULAR_SLOTS;
            uint32_t pIdx = positionLoop.mindex % CL_CIRCULAR_SLOTS;
            uint32_t vLIdx = (velocityLoop.mindex++) % CL_CIRCULAR_SLOTS;
            uint32_t vIdx = velocityLoop.mindex % CL_CIRCULAR_SLOTS;
            // uint32_t tqLIdx = (torqueLoop.mindex++) % CL_CIRCULAR_SLOTS;
            uint32_t tqIdx = torqueLoop.mindex % CL_CIRCULAR_SLOTS;
            float dt_s = (tlog_lastRecordedInteration - tlog_now) / 1.0e6;
            int encoder = global.rotorVal;
            positionLoop.measured[pIdx] = encoder;
            velocityLoop.measured[vIdx] = (encoder - positionLoop.measured[pLIdx]) * BITS_TO_ROTATIONS / (dt_s);
            torqueLoop.measured[tqIdx] = (velocityLoop.measured[vIdx] - velocityLoop.measured[vLIdx]) / (dt_s);
            tlog_lastRecordedInteration = tlog_now;
            xTaskNotifyIndexed(velocityControlLoopTask, CL_NOTIF_INDEX, 1, eSetValueWithOverwrite);
            xTaskNotifyIndexed(positionControlLoopTask, CL_NOTIF_INDEX, 1, eSetValueWithOverwrite);


            float targetTorque = *pTargetTorque;
            //check within softwaare limits, but run hardware limits
            assert( ( -SL_MAX_TORQUE <= targetTorque ) && ( targetTorque <= SL_MAX_TORQUE ) ); 

            if(global.controlMethod >= TORQUE_CONTROL){
                float tqMagnitude = fabsf( targetTorque );
                if(tqMagnitude > SL_MAX_TORQUE){
                    tqMagnitude = SL_MAX_TORQUE;
                }
                float targetDuty = LMAP(tqMagnitude, MOTOR_SPEC_MIN_TORQUE, minDuty, MOTOR_SPEC_MAX_TORQUE, maxDuty);
                for(int i=2; i>-1; i--){
                    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0, (1.0 - targetDuty)*(activePwmPeriod/2.0)));
                }
                global.dir = (targetTorque < 0) ? (5) : (2);
            }
        } else {
            xTaskNotifyIndexed(velocityControlLoopTask, CL_NOTIF_INDEX, 0, eSetValueWithOverwrite);
            xTaskNotifyIndexed(positionControlLoopTask, CL_NOTIF_INDEX, 0, eSetValueWithOverwrite);
        }
    }
}

/*Num ticks of CL_TIMER per Velocity_CL period*/
#define VELOCITY_CL_COUNT (CL_TIMER_FREQ_HZ / VELOCITY_CL_FREQ_HZ)
/* WRITES ONLY TO global. targetTorque, velocityLoop. lastError, velocityLoop. netError, AND velocityLoop. eidex */
void IRAM_ATTR velocityControlLoop(void* pointerToTarget) { 
    CLEAR_ALL_NOTIFS(NULL);
    /*POSITIVE ERROR means more ways to go, negative means overshot*/
    
    /*Filling in the scaffold form*/
    velocityLoop.timerConfig.intr_priority = VELOCITY_LOOP_TIMER_INTR_PRIORITY;
    velocityLoop.alarmConfig.alarm_count =VELOCITY_CL_COUNT;
    velocityLoop.callbackEvent.on_alarm = velocityCtrlCBK;

    /*Submitting the form*/
    ESP_ERROR_CHECK(gptimer_new_timer(&velocityLoop.timerConfig, &velocityLoop.timer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(velocityLoop.timer, &velocityLoop.alarmConfig));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(velocityLoop.timer, &velocityLoop.callbackEvent, &velocityLoop));
    ESP_ERROR_CHECK(gptimer_enable(velocityLoop.timer));
    ESP_ERROR_CHECK(gptimer_get_resolution(velocityLoop.timer, (uint32_t*) &velocityLoop.freq));
    float dt_s  = VELOCITY_CL_COUNT / velocityLoop.freq;
    ESP_LOGI("vLoop", "Frequency: %d", velocityLoop.freq / VELOCITY_CL_COUNT);
    /*CONISDER case from motor stall - to SL_MIN_VELOCITY*/
    gVar_t* pGlobalVar = (gVar_t*) pointerToTarget;
    float* pTargetVelocity = &((pGlobalVar) -> targetVelocity);

    for(;;){
        /* +error = ahead of target ccw*/
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY); /*Wait for GPTImer call*/
        volatile float targetVelocity = *pTargetVelocity;
        assert( ( -SL_MAX_VELOCITY <= targetVelocity ) && ( targetVelocity <= -SL_MAX_VELOCITY ) );

        if(global.controlMethod >= VELOCITY_CONTROL){
            // int tNow = SNAP();
            // taskENTER_CRITICAL( & sensorMux );
            // // uint32_t dt  = pGlobalVar-> tlog_readAS5600 - pGlobalVar-> tlog_trailingReadAS5600;
            // uint32_t tLastRead  = pGlobalVar-> tlog_readAS5600;
            // taskEXIT_CRITICAL( & sensorMux );

            // if(tNow - tLastRead < ACCEPTABLE_I2C_READ_WINDOW) {
            /*Check if data is recent*/
            if (ulTaskNotifyValueClearIndexed(NULL, CL_NOTIF_INDEX, 0xffffffff) != 0){
                uint32_t vidx = velocityLoop.mindex;
                uint32_t eidx = velocityLoop.eindex++; //only this loop touches it
                float errorVel =targetVelocity - velocityLoop.measured[vidx % CL_CIRCULAR_SLOTS]; //measured updated in gsn?
                float prevError = velocityLoop.lastError[eidx % CL_CIRCULAR_SLOTS];
                float currentNetError = velocityLoop.netError + errorVel * dt_s;
                
                float errorP = velocityLoop.kp * errorVel;
                float errorI = velocityLoop.ki * currentNetError; /*-area*k, */
                float errorD = velocityLoop.kd * (errorVel - prevError) / dt_s; 
                //subtract the slope (for a + slope, error should be negative)
                
                float errorTotal  = errorP + errorI + errorD; //⍺
                if (errorTotal > SL_MAX_TORQUE) {
                    errorTotal = SL_MAX_TORQUE;
                    if( !velocityLoop.overIntegration) {
                        velocityLoop.netError = currentNetError;
                    }
                } else if(errorTotal < -SL_MAX_TORQUE){
                    errorTotal = -SL_MAX_TORQUE;
                    if( !velocityLoop.overIntegration) {
                        velocityLoop.netError = currentNetError;
                    }       
                }
                //Push targets to higher loops
                pGlobalVar -> targetTorque = errorTotal;
                velocityLoop.lastError[velocityLoop.eindex % CL_CIRCULAR_SLOTS] = errorVel; //save new error
                
            }
        }
    }
}

/* WRITES ONLY TO global. targetTorque, positionLoop. lastError, positionLoop. eidex, AND positionLoop. netError */
void IRAM_ATTR positionControlLoop(void* pointerToTarget) { //Backend Function, ASSES/ASSERT TARGETS IN FRONTEND
    CLEAR_ALL_NOTIFS(NULL);
    #define POSITION_CL_COUNT (CL_TIMER_FREQ_HZ / POSITION_CL_FREQ_HZ)
    positionLoop.timerConfig.intr_priority = POSITION_LOOP_TIMER_INTR_PRIORITY;
    positionLoop.alarmConfig.alarm_count =POSITION_CL_COUNT;
    positionLoop.callbackEvent.on_alarm = positionCtrlCBK;
    ESP_ERROR_CHECK(gptimer_new_timer(&positionLoop.timerConfig, &positionLoop.timer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(positionLoop.timer, &positionLoop.alarmConfig));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(positionLoop.timer, &positionLoop.callbackEvent, &positionLoop));
    ESP_ERROR_CHECK(gptimer_enable(positionLoop.timer));
    ESP_ERROR_CHECK(gptimer_get_resolution(positionLoop.timer, (uint32_t*) &positionLoop.freq));
    float dt_s  = POSITION_CL_COUNT / positionLoop.freq;
    ESP_LOGI("pLoop", "Frequency: %d", positionLoop.freq / POSITION_CL_COUNT);

    /*CONISDER case from motor stall - to SL_MIN_POSITION*/
    gVar_t* pGlobalVar = (gVar_t*) pointerToTarget;
    int* pTargetPosition = &((pGlobalVar) -> targetPosition_BiPS);
    
    for(;;){
        ulTaskNotifyTake( pdTRUE, portMAX_DELAY);
        int targetPosition = *pTargetPosition;

        if(global.controlMethod >= POSITION_CONTROL ){
            // int tNow = SNAP();
            // taskENTER_CRITICAL( & sensorMux );
            // // uint32_t dt  = pGlobalVar-> tlog_readAS5600 - pGlobalVar-> tlog_trailingReadAS5600;
            // uint32_t tLastRead  = pGlobalVar-> tlog_readAS5600;
            // taskEXIT_CRITICAL( & sensorMux );

            // if(tNow - tLastRead < ACCEPTABLE_I2C_READ_WINDOW) {
            /*Check if data is recent*/
            if (ulTaskNotifyValueClearIndexed(NULL, CL_NOTIF_INDEX, 0xffffffff) != 0){
                uint32_t pidx = positionLoop.mindex;
                uint32_t eidx = positionLoop.eindex++; 
                float errorVel =targetPosition- positionLoop.measured[pidx % CL_CIRCULAR_SLOTS]; //measured updated in gsn?
                float prevError = positionLoop.lastError[eidx % CL_CIRCULAR_SLOTS];
                float currentNetError = positionLoop.netError + errorVel * dt_s;

                float errorP = positionLoop.kp * errorVel;
                float errorI = positionLoop.ki * currentNetError; /*-area*k, */   
                float errorD = positionLoop.kd * (errorVel - prevError) / dt_s; //subtract the slope (for a + slope, error should be negative)
                float errorTotal  = (errorP + errorI + errorD) * BITS_TO_ROTATIONS; //⍺
                if (errorTotal > SL_MAX_VELOCITY) {
                    errorTotal = SL_MAX_VELOCITY;
                    if( !positionLoop.overIntegration) {
                        positionLoop.netError = currentNetError;
                    }
                } else if(errorTotal < -SL_MAX_VELOCITY){
                    errorTotal = -SL_MAX_VELOCITY;
                    if( !positionLoop.overIntegration) {
                        positionLoop.netError = currentNetError;
                    }       
                }
                //Push targets to higher loops
                pGlobalVar -> targetVelocity = errorTotal;
                //save new error in new slot (index incremented via ++)
                positionLoop.lastError[positionLoop.eindex % CL_CIRCULAR_SLOTS] = errorVel; 
            }
        }
    }
}

/*#################### GPTIMER CALLBACKS #################### */
bool IRAM_ATTR torqueCtrlCBK (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    BaseType_t bigTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(torqueControlLoopTask, &bigTaskWoken);
    return (bool)bigTaskWoken;
};
bool IRAM_ATTR velocityCtrlCBK (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    BaseType_t bigTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(velocityControlLoopTask, &bigTaskWoken);
    return (bool)bigTaskWoken;
};
bool IRAM_ATTR positionCtrlCBK (gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx) {
    BaseType_t bigTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(positionControlLoopTask, &bigTaskWoken);
    return (bool) bigTaskWoken;
}
