# Step Motor Control

一个简单的步进电机控制程序  

## 硬件

### 使用的实物

- 主控使用STM32F103C8T6
- 电驱使用A4988芯片
- 42步进电机，13一组，46一组

### 接线

- PB13 -> DIR
- PB14 -> ENABLE
- A8 -> STEP

## 算法

- 梯形加减速算法
- 非阻塞性控制

## 程序结构

`./Core/Src/main.c`

-> `Dir_t` : 电机转动方向取值  
-> `MotorState_t` : 电机状态：停转，加速，匀速，减速  
-> `StepperCtrl_t` : 电机结构体  

- 这里没有严格使用OOP思想写电机结构体，这个程序是本人一时兴起花了一下午写出来的，这块只使用了一个电机，所以没有把`*htim`等东西写到struct里边

-> `Motor_Step_Trapezoid()` : 电机步进函数  

- 如果想接入更多电机，你可能得写一个新的`Motor_Step_Trapezoid()`，因为底层的硬件在这里面被写死了

## 使用本程序

请见`./Core/Src/main.c`->`void Motor_Step_Trapezoid(float Target_Speed, uint32_t Total_Steps, uint32_t Accel_Steps, uint32_t Decel_Steps, Dir_t Dir)`  

- `float Target_Speed` : 目标速度，实测可取`30-6000`，单位°/s
- `uint32_t Total_Steps` : 总步数，我用的电机步进角1.8°，不细分
- `uint32_t Accel_Steps` : 加速步数，含于总步数中
- `uint32_t Decel_Steps` : 减速步数，含于总步数中
- `Dir_t Dir` : 转动方向，取值`Dir1`,`Dir2`
