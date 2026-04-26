#include "pid.h"

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float int_max, float out_max)
{
    pid->Kp           = kp;
    pid->Ki           = ki;
    pid->Kd           = kd;
    pid->integral     = 0.0f;
    pid->last_error   = 0.0f;
    pid->integral_max = int_max;
    pid->output_max   = out_max;
}

float PID_Compute(PID_t *pid, float target, float actual)
{
    float error = target - actual;

    pid->integral += error;
    if (pid->integral >  pid->integral_max) pid->integral =  pid->integral_max;
    if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;

    float output = pid->Kp * error
                 + pid->Ki * pid->integral
                 + pid->Kd * (error - pid->last_error);

    pid->last_error = error;

    if (output >  pid->output_max) output =  pid->output_max;
    if (output < -pid->output_max) output = -pid->output_max;

    return output;
}

void PID_Reset(PID_t *pid)
{
    pid->integral   = 0.0f;
    pid->last_error = 0.0f;
}