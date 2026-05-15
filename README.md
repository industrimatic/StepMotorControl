# Step Motor Control

一个简单的步进电机控制程序  

## 硬件

### 使用的实物

- 主控使用STM32F103C8T6
- 电驱使用A4988
- 42步进电机，13一组，46一组
- 0.96寸OLED，驱动为SSD1306

### 接线

- PB13 -> DIR
- PB14 -> ENABLE，低电平有效
- PA8 -> STEP，上升沿有效
- PB8 -> OLED_SCL
- PB9 -> OLED_SDA

## 算法

### 使用的算法

- 梯形加减速算法
- 非阻塞性控制

### 参数设置

PB13和PB14都设置为浮空的高速推挽输出

- ENABLE这个脚初始化高电平比较好，A4988的ENABLE是低电平有效

PA8连着TIM1_CH1，这里我使用了TIM1_CH1的PWM输出功能  

- PSC设置为`72-1`
- ARR初始化为`2000-1`
- CMP初始化为`1000`

中断可以使用`TIM1 update interrupt`或`TIM1 capture compare interrupt`  

- 前者在ARR溢出时触发
- 后者在CNT == CMP 时触发

我这里使用了后者，核心思想就是在中断中使`step_now ++`，然后在`step_now == step_goal`的时候停止转动就可以了  

## 程序结构

### ./Drivers/Hardware/StepMotor.h

- 枚举类型`Dir_t`：转动方向
- 枚举类型`MotorState_t`：电机状态，停止、加速、匀速、减速
- 结构体`StepMotor_t`：电机的硬件软件状态
- 函数`StepMotor_Init()`：结构体构造函数，初始化电机状态
- 函数`StepMotor_Step_Trapezoid()`：电机转动函数
- 函数`StepMotor_Update_IT()`：检测是否步进完成的函数，在中断中调用

### ./Core/Src/main.c

1. `StepMotor_t Motor1`：声明全局电机结构体`Motor1`

2. `HAL_TIM_PWM_PulseFinishedCallback()`：中断中调用`StepMotor_Update_IT(&Motor1);`
  
3. `main()`：先调用`StepMotor_Init(&Motor1, ...);`使电机结构体初始化，然后在循环中，用`StepMotor_Step_Trapezoid(&Motor1,...)`使电机以特定速度步进特定步数，实时访问结构体的成员`Motor1.Current_Speed`等输出到OLED设备上

注意hal库中检测TIM中断通道的是`HAL_TIM_ActiveChannel`这个枚举类型而不是宏定义`HAL_CHANNEL_1`，两个值不一样
  
## 使用本程序

### 认识电机结构体

```C
typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint8_t active_channel;
    GPIO_TypeDef *DirPort;
    uint16_t DirPin;
    GPIO_TypeDef *EnPort;
    uint16_t EnPin;

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
```

这里存放着电机相关的信息，`EnPin`及其之前的都是电机的硬件信息，后面的都是软件信息，可以通过`Motor1.Current_Speed`进行获取

### 初始化

首先你需要在`main.c`中以全局变量的形式声明电机

```C
StepMotor_t Motor1;
```

然后在`main`函数中使用`StepMotor_Init()`初始化电机的结构体

`./Drivers/Hardware/StepMotor.h`->`void StepMotor_Init(StepMotor_t *motor, TIM_HandleTypeDef *htim, uint32_t channel, GPIO_TypeDef *dir_port, uint16_t dir_pin, GPIO_TypeDef *enable_port, uint16_t enable_pin, float start_speed);`

- `StepMotor_t *motor`：电机结构体，指明要操作的电机
- `TIM_HandleTypeDef *htim`：电机使用的TIM，我这里是TIM1，所以传入了`htim1`
- `uint32_t channel`：电机使用的TIM_CH，我这里是TIM1_CH1，所以传入了`TIM_CHANNEL_1`
- `GPIO_TypeDef *dir_port`：DIR脚的GPIO
- `uint16_t dir_pin`：DIR脚的GPIO_PIN
- `GPIO_TypeDef *enable_port`：EN脚的GPIO
- `uint16_t enable_pin`：EN脚的GPIO_PIN
- `float start_speed`：电机最小速度

### 操作电机

`./Drivers/Hardware/StepMotor.h`->`void StepMotor_Step_Trapezoid(StepMotor_t *motor, float Target_Speed, uint32_t Total_Steps, uint32_t Accel_Steps, uint32_t Decel_Steps, Dir_t Dir)`

- `StepMotor_t *motor`：电机结构体，指明要操作的电机
- `float Target_Speed`：目标速度，根据硬件的不同可取的范围也不同，我这里实测可取`30-6000`，单位°/s
- `uint32_t Total_Steps`：总步数，我用的电机步进角1.8°，不细分
- `uint32_t Accel_Steps`：加速步数，含于总步数中
- `uint32_t Decel_Steps`：减速步数，含于总步数中
- `Dir_t Dir`：转动方向，取值`DIR1`,`DIR2`

### 中断回调

根据你使用的不同中断有不同的回调函数，我这里用的是`TIM1 capture compare interrupt`所以使用`void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)`

```C
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == Motor1.htim->Instance)
  {
    if (htim->Channel == Motor1.active_channel)
    {
      StepMotor_Update_IT(&Motor1);
    }
  }
}
```

在其中先判断`Instance`和`Channel`之后，调用`StepMotor_Update_IT()`即可

### 注意事项

在`StepMotor_Step_Trapezoid()`和`StepMotor_Update_IT()`中我写了如下代码

```C
uint32_t ARR = (uint32_t)(1800000.0f / motor->Current_Speed);
```

`1800000.0f`的意思是$1.8 \times 1000000$，其中$1.8$是指步距角$1.8\degree$，$1000000$是TIM1的计数频率

当PWM的频率为$f$，步距角为$D$，则电机转动速度为$V=Df$，单位$\degree/s$。

所以当设定速度为 $V\degree/s$ 时，$f = V/D$

又由于PWM周期=ARR+1/TIM频率，即

$$\frac{1}{f}=\frac{ARR+1}{1,000,000}$$

所以

$$ARR+1=\frac{1,000,000}{f} =\frac{1,000,000D}{V}= \frac{1,800,000}{V}$$

因此代码中速度与 ARR 的转换常数为 1,800,000，如果你设定的步距角和TIM频率不一样，请调整这个常数
