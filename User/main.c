#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "Timer.h"
#include "Key.h"
#include "RP.h"
#include "Motor.h"
#include "Encoder.h"
#include "Serial.h"
#include <stdlib.h>
#include <string.h>
#include <PWM.h>
uint8_t KeyNum;

// 双电机相关的全局变量
int32_t Encoder1_Count = 0, Encoder2_Count = 0;
int16_t Speed1 = 0, Speed2 = 0;

// 电机1 PID变量
float Target, Actual, Out;
float Kp, Ki, Kd;
float Error0, Error1, Error2;

// 电机2 PID变量
float Target2, Actual2, Out2;
float Kp2, Ki2, Kd2;
float Error2_0, Error2_1, Error2_2;

// 串口通信变量
char UART_RxBuffer[32];
uint8_t UART_RxIndex = 0;
uint8_t UART_RxFlag = 0;

// OLED显示优化变量
int16_t last_Speed1 = -9999;
int16_t last_Speed2 = -9999;
int16_t last_Target1 = -9999;
int16_t last_Target2 = -9999;
int32_t last_Encoder1 = 999999;
int32_t last_Encoder2 = 999999;

// 函数声明
void Process_UART_Command(void);
void Send_Data_To_PC(void);

int main(void)
{
    Serial_Init();
    Motor_Init();
	PWM_Init();
    
    Serial_Printf("=== Motor and Hardware Test ===\r\n");
    
    // 手动设置所有引脚，绕过Motor_SetPWM函数
    // 设置电机1方向为正转
    GPIO_ResetBits(GPIOB, GPIO_Pin_12); // AIN1 = 0
    GPIO_SetBits(GPIOB, GPIO_Pin_13);   // AIN2 = 1
    
    // 设置电机2方向为正转
    GPIO_ResetBits(GPIOB, GPIO_Pin_14); // BIN1 = 0
    GPIO_SetBits(GPIOB, GPIO_Pin_15);   // BIN2 = 1
    
    Serial_Printf("Direction pins set for forward rotation.\r\n");
    
    // 设置PWM为50%
    PWM_SetCompare1(50); // 给PA2 (电机1) 50%的PWM
    PWM_SetCompare2(50); // 给PA3 (电机2) 50%的PWM
    
    Serial_Printf("PWM set to 50%% on both channels.\r\n");
    Serial_Printf("Motor should be spinning NOW.\r\n");
    
    while (1)
    {
        // 每2秒打印一次状态
        Serial_Printf("Status: Motors should be running...\r\n");
        Delay_ms(2000);
    }
}




/*
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
    {
        // My_LED_Turn(); // 可以注释掉，避免干扰串口
        
        static uint16_t Count;
        Count++;
        
        if (Count >= 1)    // 10ms执行一次
        {
            Count = 0;
            
            // 读取编码器值
            Speed1 = (int16_t)TIM_GetCounter(TIM3);
            Speed2 = (int16_t)TIM_GetCounter(TIM4);
            
            // <--- 添加调试打印 ---
            static uint16_t debug_counter = 0;
            debug_counter++;
            if (debug_counter >= 100) // 每1秒打印一次
            {
                debug_counter = 0;
                Serial_Printf("Running! S1:%d S2:%d\r\n", Speed1, Speed2);
            }
            // --- 调试结束 ---
        }
        
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    }
}
*/


// 处理串口命令 @speedXX%
void Process_UART_Command(void)
{
    if (strstr(UART_RxBuffer, "@speed") != NULL) {
        int speed = atoi(UART_RxBuffer + 6);
        if (speed > 100) speed = 100;
        if (speed < -100) speed = -100;
        
        // 设置目标速度
        Target = speed * 8.16f;
        Target2 = speed * 8.16f;
        
        Serial_Printf("Target speed set to: %d%%\r\n", speed);
    }
    
    UART_RxIndex = 0;
    memset(UART_RxBuffer, 0, sizeof(UART_RxBuffer));
}

// 发送数据到VOFA+用于可视化
void Send_Data_To_PC(void)
{
    // VOFA+格式：数据1,数据2,数据3,数据4\r\n
    Serial_Printf("%d,%d,%d,%d\r\n", Speed1, (int)Target, Speed2, (int)Target2);
}