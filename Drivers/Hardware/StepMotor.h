#ifndef __STEPMOTOR_H
#define __STEPMOTOR_H

#include "stm32f1xx.h"

typedef enum
{
    DIR1 = GPIO_PIN_SET,
    DIR2 = GPIO_PIN_RESET
} Dir_t;

typedef enum
{
    MOTOR_STOP = 0,
    MOTOR_ACCEL,
    MOTOR_CRUISE,
    MOTOR_DECEL
} MotorState_t;

typedef struct
{
    // 硬件配置
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint8_t active_channel;
    GPIO_TypeDef *DirPort;
    uint16_t DirPin;
    GPIO_TypeDef *EnPort;
    uint16_t EnPin;

    // 状态成员
    uint32_t Step_Goal;
    uint32_t Step_Now;
    MotorState_t State;

    float Start_Speed;
    float Target_Speed;
    float Current_Speed;

    uint32_t Accel_Steps;
    uint32_t Decel_Steps;

    float Accel_Rate;
    float Decel_Rate;
} StepMotor_t;

void StepMotor_Init(StepMotor_t *motor, TIM_HandleTypeDef *htim, uint32_t channel,
                    GPIO_TypeDef *dir_port, uint16_t dir_pin,
                    GPIO_TypeDef *enable_port, uint16_t enable_pin, float start_speed);

void StepMotor_Step_Trapezoid(StepMotor_t *motor, float Target_Speed, uint32_t Total_Steps, uint32_t Accel_Steps, uint32_t Decel_Steps, Dir_t Dir);
void StepMotor_Update_IT(StepMotor_t *motor);
#endif
