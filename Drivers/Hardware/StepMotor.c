
#include "StepMotor.h"

void StepMotor_Init(StepMotor_t *motor, TIM_HandleTypeDef *htim, uint32_t channel,
                    GPIO_TypeDef *dir_port, uint16_t dir_pin,
                    GPIO_TypeDef *enable_port, uint16_t enable_pin, float start_speed)
{
    motor->htim = htim;
    motor->channel = channel;
    motor->DirPort = dir_port;
    motor->DirPin = dir_pin;
    motor->EnPort = enable_port;
    motor->EnPin = enable_pin;
    motor->Start_Speed = start_speed;

    motor->State = MOTOR_STOP;
    motor->Step_Now = 0;

    switch (channel)
    {
    case TIM_CHANNEL_1:
        motor->active_channel = HAL_TIM_ACTIVE_CHANNEL_1;
        break;

    case TIM_CHANNEL_2:
        motor->active_channel = HAL_TIM_ACTIVE_CHANNEL_2;
        break;

    case TIM_CHANNEL_3:
        motor->active_channel = HAL_TIM_ACTIVE_CHANNEL_3;
        break;

    case TIM_CHANNEL_4:
        motor->active_channel = HAL_TIM_ACTIVE_CHANNEL_4;
        break;

    default:
        break;
    }

    HAL_GPIO_WritePin(enable_port, enable_pin, GPIO_PIN_SET);
}

void StepMotor_Step_Trapezoid(StepMotor_t *motor, float Target_Speed, uint32_t Total_Steps, uint32_t Accel_Steps, uint32_t Decel_Steps, Dir_t Dir)
{
    if (motor->State != MOTOR_STOP)
        return;
    else
    {
        motor->Step_Goal = Total_Steps;
        motor->Step_Now = 0;

        motor->Current_Speed = motor->Start_Speed;
        if (Target_Speed < motor->Start_Speed)
            motor->Target_Speed = motor->Start_Speed;
        else
            motor->Target_Speed = Target_Speed;

        if (Accel_Steps + Decel_Steps >= Total_Steps)
        {
            motor->Accel_Steps = Total_Steps / 2;
            motor->Decel_Steps = Total_Steps - motor->Accel_Steps;
        }
        else
        {
            motor->Accel_Steps = Accel_Steps;
            motor->Decel_Steps = Decel_Steps;
        }

        motor->Accel_Rate = (motor->Target_Speed - motor->Start_Speed) / Accel_Steps;
        motor->Decel_Rate = (motor->Target_Speed - motor->Start_Speed) / Decel_Steps;

        uint32_t ARR = (uint32_t)(1800000.0f / motor->Current_Speed);
        __HAL_TIM_SET_AUTORELOAD(motor->htim, ARR - 1);
        __HAL_TIM_SET_COMPARE(motor->htim, motor->channel, ARR / 2);
        __HAL_TIM_SET_COUNTER(motor->htim, 0);

        motor->State = MOTOR_ACCEL;
        HAL_GPIO_WritePin(motor->DirPort, motor->DirPin, (GPIO_PinState)Dir);
        HAL_GPIO_WritePin(motor->EnPort, motor->EnPin, GPIO_PIN_RESET);
        HAL_TIM_PWM_Start_IT(motor->htim, motor->channel);
    }
}

void StepMotor_Update_IT(StepMotor_t *motor)
{
    motor->Step_Now++;
    if (motor->Step_Now >= motor->Step_Goal)
    {
        HAL_TIM_PWM_Stop_IT(motor->htim, motor->channel);
        HAL_GPIO_WritePin(motor->EnPort, motor->EnPin, GPIO_PIN_SET);
        motor->State = MOTOR_STOP;
    }
    else
    {
        if (motor->Step_Now <= motor->Accel_Steps)
        {
            motor->State = MOTOR_ACCEL;
            motor->Current_Speed += motor->Accel_Rate;
        }
        else if (motor->Step_Now >= (motor->Step_Goal - motor->Decel_Steps))
        {
            motor->State = MOTOR_DECEL;
            motor->Current_Speed -= motor->Decel_Rate;
            if (motor->Current_Speed < motor->Start_Speed)
                motor->Current_Speed = motor->Start_Speed;
        }
        else
        {
            motor->State = MOTOR_CRUISE;
            motor->Current_Speed = motor->Target_Speed;
        }

        uint32_t new_ARR = (uint32_t)(1800000.0f / motor->Current_Speed);

        if (new_ARR > 65535)
            new_ARR = 65535;
        if (new_ARR < 100)
            new_ARR = 100;

        __HAL_TIM_SET_AUTORELOAD(motor->htim, new_ARR - 1);
        __HAL_TIM_SET_COMPARE(motor->htim, motor->channel, new_ARR / 2);
    }
}
