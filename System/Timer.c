#include "stm32f10x.h"                  // Device header

/**
 * 函数名：Timer_Init
 * 功  能：初始化定时器，10ms中断一次
 * 用  途：为PID控制提供固定的时间基准
 * 计  算：72MHz / 72 / 10000 = 100Hz，即10ms中断一次
 */
void Timer_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);    // 开启TIM1时钟
    
    TIM_InternalClockConfig(TIM1);    // 选择TIM1为内部时钟
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;     // 时钟不分频
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseInitStructure.TIM_Period = 10000 - 1;               // 10ms定时 (72MHz/72*10000)
    TIM_TimeBaseInitStructure.TIM_Prescaler = 72 - 1;                // 预分频72，得到1MHz时钟
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStructure);
    
    TIM_ClearFlag(TIM1, TIM_FLAG_Update);    // 清除更新标志位
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE); // 使能更新中断
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);    // 中断分组2
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;      // TIM1更新中断
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         // 使能中断通道
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 抢占优先级1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;      // 子优先级1
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM1, ENABLE);    // 使能TIM1
}



/*
void TIM1_UP_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1, TIM_IT_Update) == SET)
	{
		
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
	}
}
*/
