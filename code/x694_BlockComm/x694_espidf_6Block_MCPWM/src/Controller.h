constexpr float kPID[3][3] = {
    { 1, 1, 1 }, /*Position*/
    { 1.1, 0,0 }, /*Velocity {kp, ki, kd}*/
    { 1, 1, 1 } /*Acceleration*/
};

void mathItOut(void * startTick4); //updates arrrays with new ifo
void setTorque(float targetTorque);
void setVelocity(float targetVelocity);
void setPosition(float targetPosition);