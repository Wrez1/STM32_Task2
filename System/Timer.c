#include "stm32f10x.h"                  // Device header


// 定时器中断服务函数（在main.c中实现，这里声明一下）
extern void TIM1_UP_IRQHandler(void); 


/// Timer.c
void Timer_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InternalClockConfig(TIM1);
    
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_RepetitionCounter = 0;
    
    // <--- 修改这里，改为10ms中断 ---
    TIM_InitStructure.TIM_Prescaler = 7200 - 1; // 7200分频
    TIM_InitStructure.TIM_Period = 100 - 1;    // 计数值100
    
    TIM_TimeBaseInit(TIM1, &TIM_InitStructure);
    
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    TIM_Cmd(TIM1, ENABLE);
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
