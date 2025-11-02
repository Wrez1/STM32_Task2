#include "stm32f10x.h"                  // Device header

/**
 * 函数名：PWM_Init
 * 功  能：初始化双通道PWM输出
 * 硬件：PWM1-PA0(TIM2_CH1), PWM2-PA1(TIM2_CH2)
 * 频率：约10kHz (72MHz/36/100 = 20kHz, 实际为10kHz因为计数模式)
 * 说  明：配置TIM2产生两路PWM信号控制电机转速
 */
void PWM_Init(void)
{
    /*开启时钟*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);    // 开启TIM2时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);    // 开启GPIOA时钟
    
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /*PWM1引脚初始化 - PA0(TIM2_CH1) 控制电机1*/
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;          // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;                // PA0
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /*PWM2引脚初始化 - PA1(TIM2_CH2) 控制电机2*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;                // PA1
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    /*配置时钟源*/
    TIM_InternalClockConfig(TIM2);    // 选择TIM2为内部时钟
    
    /*TIM2时基单元初始化*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟不分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseInitStructure.TIM_Period = 100 - 1;                 // 计数周期100，PWM频率约10kHz
    TIM_TimeBaseInitStructure.TIM_Prescaler = 36 - 1;                // 预分频36，72MHz/36=2MHz
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);
    
    /*输出比较通道初始化*/
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCStructInit(&TIM_OCInitStructure);                        // 结构体初始化
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;               // PWM模式1
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;       // 输出极性高
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;   // 输出使能
    TIM_OCInitStructure.TIM_Pulse = 0;                              // 初始PWM占空比为0
    
    TIM_OC1Init(TIM2, &TIM_OCInitStructure);  // 初始化通道1 (PA0)
    TIM_OC2Init(TIM2, &TIM_OCInitStructure);  // 初始化通道2 (PA1)
    
    /*使能预装载*/
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);  // 通道1预装载使能
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);  // 通道2预装载使能
    TIM_ARRPreloadConfig(TIM2, ENABLE);                // 自动重装载预装载使能
    
    /*使能TIM2*/
    TIM_Cmd(TIM2, ENABLE);
}

/**
  * 函    数：PWM设置CCR
  * 参    数：Compare 要写入的CCR的值，范围：0~100
  * 返 回 值：无
  * 注意事项：CCR和ARR共同决定占空比，此函数仅设置CCR的值，并不直接是占空比
  *           占空比Duty = CCR / (ARR + 1)
  */
void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM2, Compare);		//设置CCR1的值
}

/**
 * 函数名：PWM_SetCompare2
 * 功  能：设置PWM通道2的占空比
 * 参  数：Compare - 比较值(0~99)
 * 说  明：控制电机2的转速
 */
void PWM_SetCompare2(uint16_t Compare)
{
    TIM_SetCompare2(TIM2, Compare);  // 设置TIM2通道2的比较寄存器值
}
