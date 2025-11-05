#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "Key.h"
#include "Timer.h"
#include "Encoder.h"
#include "Motor.h"
#include "Serial.h"
#include "vofa.h"
#include <stdlib.h>
#include <string.h>

/*------------------- 全局变量定义 -------------------*/
volatile uint8_t Current_Mode = 0; // 0: 速度环, 1: 位置环 (添加volatile)
uint8_t KeyNum;

// --- 新的、独立的串口接收系统 ---
#define UART_RX_BUFFER_SIZE 32
volatile char UART_RxBuffer_New[UART_RX_BUFFER_SIZE];
volatile uint8_t UART_RxIndex_New = 0;
volatile uint8_t UART_RxFlag_New = 0; // 接收完成标志

// 编码器
volatile int16_t Speed1 = 0; // (添加volatile)
int32_t Encoder1_Count = 0;

// --- 任务一：速度环PID变量 ---
volatile float Target_Speed = 0;  // 目标速度 (添加volatile)
volatile float Actual_Speed = 0;  // 实际速度 (添加volatile)
volatile float PID_Out_Speed = 0; // 速度环PID输出 (添加volatile)

// PID参数
float Kp_speed = 0.5f;
float Ki_speed = 0.1f;
float Kd_speed = 0.05f;

float Error_Speed_Now = 0;
float Error_Speed_Last = 0;

/*------------------- 函数声明 -------------------*/
void Process_UART_Command_Stable(char* cmd);
void Send_Data_To_PC(void);

/**
  * @brief  主函数
  */
int main(void)
{
    // 模块初始化
    OLED_Init();
    Key_Init();
    Timer_Init();
    Encoder1_Init();
    Motor_Init();
    Serial_Init(); // 只用它来初始化USART硬件，中断由我们自己的函数接管
    
    while (1)
    {
        // 1. 按键检测与模式切换
        KeyNum = Key_GetNum();
        if (KeyNum == 1)
        {
            Current_Mode = !Current_Mode;
            Target_Speed = 0;
            PID_Out_Speed = 0;
        }
        
        // 2. 【新的】处理串口命令
        if (UART_RxFlag_New)
        {
            if (Current_Mode == 0)
            {
                Process_UART_Command_Stable((char*)UART_RxBuffer_New);
            }
            UART_RxFlag_New = 0; // 清除标志位
            
            // 手动清空缓冲区
            for (uint16_t i = 0; i < UART_RX_BUFFER_SIZE; i++) {
                UART_RxBuffer_New[i] = 0;
            }
        }
        
        // 3. 更新OLED显示
        OLED_Clear();
        OLED_ShowString(1, 0, "Mode:", OLED_8X16);
        if (Current_Mode == 0) {
            OLED_ShowString(41, 0, "Speed", OLED_8X16);
        } else {
            OLED_ShowString(41, 0, "Position", OLED_8X16);
        }
        OLED_ShowString(1, 16, "T:", OLED_8X16);
        OLED_ShowSignedNum(17, 16, (int16_t)Target_Speed, 4, OLED_8X16);
        OLED_ShowString(65, 16, "A:", OLED_8X16);
        OLED_ShowSignedNum(81, 16, Speed1, 4, OLED_8X16);
        OLED_Update();
        
        // 4. 定时发送数据到VOFA+
        Send_Data_To_PC();
        
        Delay_ms(10);
    }
}

/**
  * @brief  【新的】我们自己的串口中断服务函数，覆盖Serial.c里的
  */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t rx_data = USART_ReceiveData(USART1);
        
        if (rx_data == '\r') // 检测到回车
        {
            UART_RxBuffer_New[UART_RxIndex_New] = '\0'; // 添加字符串结束符
            UART_RxFlag_New = 1; // 设置标志位，通知主循环处理
            UART_RxIndex_New = 0; // 重置索引
        }
        else if (rx_data != '\n') // 忽略换行符
        {
            UART_RxBuffer_New[UART_RxIndex_New] = rx_data;
            UART_RxIndex_New++;
            if (UART_RxIndex_New >= UART_RX_BUFFER_SIZE) // 防止溢出
            {
                UART_RxIndex_New = 0;
            }
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

/**
  * @brief  定时器1中断服务函数 (核心控制循环, 10ms执行一次)
  */
void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
    {
        // --- 读取编码器数据 ---
        Speed1 = Encoder1_Get();
        Encoder1_Count += Speed1;

        // --- 任务一：速度环 ---
        if (Current_Mode == 0) 
        {
            Actual_Speed = (float)Speed1;
            Error_Speed_Now = Target_Speed - Actual_Speed;
            
            // 增量式PID
            PID_Out_Speed += (Kp_speed * (Error_Speed_Now - Error_Speed_Last)) + (Ki_speed * Error_Speed_Now);
            
            Error_Speed_Last = Error_Speed_Now;
            
            // 限制输出
            if (PID_Out_Speed > 100) PID_Out_Speed = 100;
            if (PID_Out_Speed < -100) PID_Out_Speed = -100;
            
            // 控制电机1
            Motor_SetPWM(1, (int8_t)PID_Out_Speed);
        }
        else
        {
            Motor_SetPWM(1, 0);
        }
        
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
    }
}

/**
  * @brief  处理串口命令 (稳定版)
  */
void Process_UART_Command_Stable(char* cmd)
{
    if (cmd[0] == '@' && cmd[1] == 's' && cmd[2] == 'p' && cmd[3] == 'e' && cmd[4] == 'e' && cmd[5] == 'd')
    {
        int speed_percent = 0;
        uint8_t i = 6;
        
        // 手动解析数字
        while (cmd[i] >= '0' && cmd[i] <= '9')
        {
            speed_percent = speed_percent * 10 + (cmd[i] - '0');
            i++;
        }
        
        // 处理负号
        if (cmd[6] == '-') {
            i = 7;
            while (cmd[i] >= '0' && cmd[i] <= '9')
            {
                speed_percent = speed_percent * 10 + (cmd[i] - '0');
                i++;
            }
            speed_percent = -speed_percent;
        }
        
        if (speed_percent > 100) speed_percent = 100;
        if (speed_percent < -100) speed_percent = -100;
        
        Target_Speed = (float)speed_percent * 8.16f;
    }
}

/**
  * @brief  发送数据到VOFA+ (使用JustFloat协议)
  */
void Send_Data_To_PC(void)
{
    static uint8_t send_counter = 0;
    send_counter++;
    if (send_counter >= 5) // 5 * 10ms = 50ms
    {
        send_counter = 0;
        
        if (Current_Mode == 0) {
            float vofa_data[3];
            vofa_data[0] = Actual_Speed;
            vofa_data[1] = Target_Speed;
            vofa_data[2] = PID_Out_Speed;
            VOFA_JustFloat_Send(vofa_data, 3);
        }
    }
}
