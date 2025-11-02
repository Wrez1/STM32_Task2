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
    // 模块初始化
    OLED_Init();
    Key_Init();
    Motor_Init();
    Encoder1_Init();
	Encoder2_Init();
    RP_Init();
    Serial_Init();
    Timer_Init();
    
    // PID参数初始化
    Kp = 2.0f; Ki = 0.5f; Kd = 0.1f;    // 电机1
    Kp2 = 2.0f; Ki2 = 0.5f; Kd2 = 0.1f;  // 电机2
    Target = 0; Target2 = 0;
    
    // 初始显示固定框架
    OLED_Clear();
    OLED_Printf(0, 0, OLED_8X16, "Speed Mode      ");
    OLED_Printf(0, 16, OLED_8X16, "M1:     /      ");
    OLED_Printf(0, 32, OLED_8X16, "M2:     /      ");
    OLED_Printf(0, 48, OLED_8X16, "E1:       E2:   ");
    OLED_Update();
    
    Delay_ms(1000);
    
    uint16_t display_counter = 0;
    
    while (1)
    {
        // 处理串口命令
        if (UART_RxFlag) {
            Process_UART_Command();
            UART_RxFlag = 0;
        }
        
        // 发送数据到VOFA+
        Send_Data_To_PC();
        
        // OLED显示更新（100ms刷新一次，不闪烁）
        display_counter++;
        if (display_counter >= 10)  // 10 * 10ms = 100ms
        {
            display_counter = 0;
            
            // 只在数值变化时更新对应位置
            if (Speed1 != last_Speed1) {
                OLED_Printf(24, 16, OLED_8X16, "%+04d", Speed1);
                last_Speed1 = Speed1;
            }
            
            if ((int)Target != last_Target1) {
                OLED_Printf(72, 16, OLED_8X16, "%+04d", (int)Target);
                last_Target1 = (int)Target;
            }
            
            if (Speed2 != last_Speed2) {
                OLED_Printf(24, 32, OLED_8X16, "%+04d", Speed2);
                last_Speed2 = Speed2;
            }
            
            if ((int)Target2 != last_Target2) {
                OLED_Printf(72, 32, OLED_8X16, "%+04d", (int)Target2);
                last_Target2 = (int)Target2;
            }
            
            if (Encoder1_Count != last_Encoder1) {
                OLED_Printf(16, 48, OLED_8X16, "%+06ld", Encoder1_Count);
                last_Encoder1 = Encoder1_Count;
            }
            
            if (Encoder2_Count != last_Encoder2) {
                OLED_Printf(72, 48, OLED_8X16, "%+06ld", Encoder2_Count);
                last_Encoder2 = Encoder2_Count;
            }
            
            OLED_Update();  // 只更新变化的部分
        }
        
        Delay_ms(10);
    }
}

// 定时器中断服务函数（10ms执行一次）
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
    {
        static uint16_t Count;
        Count++;
        
        Key_Tick();
        
        if (Count >= 1)    // 10ms执行一次PID控制
        {
            Count = 0;
            
            // 读取编码器增量值
            int16_t encoder1_delta = Encoder1_Get();
            int16_t encoder2_delta = Encoder2_Get();
            
            // 更新累计计数和速度
            Encoder1_Count += encoder1_delta;
            Encoder2_Count += encoder2_delta;
            Speed1 = encoder1_delta;  // 10ms内增量即为速度
            Speed2 = encoder2_delta;
            
            // 电机1 PID控制（保持原有逻辑）
            Actual += encoder1_delta;
            Error2 = Error1;
            Error1 = Error0;
            Error0 = Target - Actual;
            
            // 增量式PID公式
            Out += Kp * (Error0 - Error1) + Ki * Error0
                   + Kd * (Error0 - 2 * Error1 + Error2);
            
            // 电机2 PID控制（速度环）
            Error2_2 = Error2_1;
            Error2_1 = Error2_0;
            Error2_0 = Target2 - Speed2;
            
            // 增量式PID公式
            Out2 += Kp2 * (Error2_0 - Error2_1) + Ki2 * Error2_0
                    + Kd2 * (Error2_0 - 2 * Error2_1 + Error2_2);
            
            // 输出限幅
            if (Out > 100) Out = 100;
            if (Out < -100) Out = -100;
            if (Out2 > 100) Out2 = 100;
            if (Out2 < -100) Out2 = -100;
            
            // 控制电机
            Motor_SetPWM(1, Out);
            Motor_SetPWM(2, Out2);
        }
        
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    }
}

// 串口接收中断
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t data = USART_ReceiveData(USART1);
        
        if (data == '\r' || data == '\n') {
            if (UART_RxIndex > 0) {
                UART_RxBuffer[UART_RxIndex] = '\0';
                UART_RxFlag = 1;  // 接收完成标志
            }
        } else if (UART_RxIndex < sizeof(UART_RxBuffer) - 1) {
            UART_RxBuffer[UART_RxIndex++] = data;
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

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