#include "stm32f10x.h"                  // Device header
#include "PWM.h"

/**
  * 函    数：直流电机初始化
  * 参    数：无
  * 返 回 值：无
  */
void Motor_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    // 电机1: PB12, PB13
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    // 电机2: PB14, PB15
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    PWM_Init();
}

/**
 * 函数名：Motor_SetPWM
 * 功  能：设置指定电机的PWM值和方向
 * 参  数：motor - 电机编号(1或2)
 *        PWM   - PWM值(-100~100，负值表示反转)
 * 说  明：通过控制方向引脚和PWM输出来控制电机转速和方向
 */
void Motor_SetPWM(uint8_t motor, int8_t PWM)
{
    if (motor == 1)  // 控制电机1
    {
        if (PWM >= 0)  // 正转
        {
            GPIO_ResetBits(GPIOB, GPIO_Pin_12);   // AIN1 = 0
            GPIO_SetBits(GPIOB, GPIO_Pin_13);     // AIN2 = 1
            PWM_SetCompare1(PWM);                 // 设置PWM占空比
        }
        else  // 反转
        {
            GPIO_SetBits(GPIOB, GPIO_Pin_12);     // AIN1 = 1
            GPIO_ResetBits(GPIOB, GPIO_Pin_13);   // AIN2 = 0
            PWM_SetCompare1(-PWM);                // PWM值为正数
        }
    }
    else if (motor == 2)  // 控制电机2
    {
        if (PWM >= 0)  // 正转
        {
            GPIO_ResetBits(GPIOB, GPIO_Pin_14);   // BIN1 = 0
            GPIO_SetBits(GPIOB, GPIO_Pin_15);     // BIN2 = 1
            PWM_SetCompare2(PWM);                 // 使用PWM通道2
        }
        else  // 反转
        {
            GPIO_SetBits(GPIOB, GPIO_Pin_14);     // BIN1 = 1
            GPIO_ResetBits(GPIOB, GPIO_Pin_15);   // BIN2 = 0
            PWM_SetCompare2(-PWM);                // PWM值为正数
        }
    }
}
