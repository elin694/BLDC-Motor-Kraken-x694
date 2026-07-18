#include "Globals.h"
#include "Controller.h"

void mathItOut(void * startTick4){ //updates arrrays with new ifo
   CLEAR_ALL_NOTIFS(NULL);
   TickType_t startTick = *(TickType_t*)startTick4;
   float dt  = estimatedI2CReadTime_us;
   
   xTaskDelayUntil(&startTick,initializationLatency);
   for(;;){
      // index shuld stay in here
      //cahgne init as5600 read &&&||||| move to next index then save
      uint32_t file1 = ulTaskNotifyTakeIndexed(0, pdTRUE, pdMS_TO_TICKS(100));
      int previousPos = global.measuredPos[(global.pindex.fetch_add(1))%cBufSize];
      float previousVel = global.measuredVel[(global.vindex.fetch_add(1))%cBufSize];
      float previousAccel = global.measuredAccel[(global.aindex.fetch_add(1))%cBufSize];
      
      uint32_t vidx = global.vindex;
      uint32_t pidx = global.pindex;
      //*assuming dir is always at ground
      // global.measuredPos[pidx%cBufSize] = global.rotorVal;
      // float newVel = global.measuredVel[vidx%cBufSize] = ((global.rotorVal-previousPos)%4096)/dt;
      // global.measuredAccel[global.aindex%cBufSize] = (newVel-previousVel)/dt;
      taskYIELD();
   }
}

void setTorque(float targetTorque){ //
    if(global.controlMethod <= TORQUE_CONTROL){
        float normalizedMagnitude = fabsf( LMAP(targetTorque, SL_MAX_TORQUE, SL_MIN_TORQUE, 1, 0) );
        if(normalizedMagnitude < minDuty || normalizedMagnitude > maxDuty){
            global.setMotorFreeSpin = true; //rmeove delay between this and freespining in the future
        } else{
        for(int i=2; i>-1; i--){
            // ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(motorH[i].comparator0,(1-magnitude)*(activePwmPeriod/2.0)));
        }
        }
        global.dir = (targetTorque < 0) ? (5) : (2);
    }
}

void setVelocity(float targetVelocity){
   for(;;){
         /* +error = ahead of target ccw*/
         if(global.controlMethod <= VELOCITY_CONTROL){
            float dt  = estimatedI2CReadTime_us;
            uint32_t vidx = global.vindex;
            float errorVel =targetVelocity- global.measuredVel[vidx%cBufSize];/////////////////
            float prevError = global.lastVelError;
            global.totalVelChange = global.totalVelChange + errorVel*dt;

            float errorP = kPID[VELOCITY_CONTROL][0]*errorVel;
            float errorI = kPID[VELOCITY_CONTROL][1]*global.totalVelChange; /*-area*k, */
            float errorD = kPID[VELOCITY_CONTROL][2]*(errorVel-prevError)/dt; //subtract the slope (for a + slope, error should be negative)
            /*finally changes set cmpVal*/
            float errorTotal  = errorP +errorI+errorD; //⍺
            if(errorTotal > maxDuty){
               errorTotal = maxDuty;
            } else if (errorTotal < -maxDuty){
               errorTotal = -maxDuty;
            }
            setTorque(errorTotal);
            ///remember to set prev Erorr and other past var
         }
      }
}

void setPosition(float targetPosition){
   for(;;){
      if(global.controlMethod <= POSITION_CONTROL){
         float dt  = estimatedI2CReadTime_us;
         uint32_t pidx = global.pindex;
         float errorPos =global.targetPosition- global.measuredPos[pidx%cBufSize];/////////////////
         float prevError = global.lastPosError;
         global.totalPosChange = global.totalPosChange + errorPos*dt;

         float errorP = kPID[POSITION_CONTROL][0]*errorPos;
         float errorI = kPID[POSITION_CONTROL][1]*global.totalPosChange; /*-area*k, */
         float errorD = kPID[POSITION_CONTROL][2]*(errorPos-prevError)/dt; //subtract the slope (for a + slope, error should be negative)

         float errorTotal  = errorP +errorI+errorD; //⍺
         #define maxBiPS (SL_MAX_VELOCITY * ROTATIONS_TO_BITS)
         if(errorTotal > maxBiPS){
            errorTotal = maxBiPS;
         } else if (errorTotal < -maxBiPS){
            errorTotal = -maxBiPS;
         }
         setVelocity(errorTotal);
         //error can theoretically go from 0 to infinity for 1 c. ->
      }
   }
}
