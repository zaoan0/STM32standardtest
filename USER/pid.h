#ifndef __PID_H
#define __PID_H

typedef struct {
    float Kp;
    float Ki;
    float Kd;
    float integral;
    float last_error;
    float integral_max;
    float output_max;
} PID_t;

void  PID_Init   (PID_t *pid, float kp, float ki, float kd,
                  float int_max, float out_max);
float PID_Compute(PID_t *pid, float target, float actual);
void  PID_Reset  (PID_t *pid);

#endif