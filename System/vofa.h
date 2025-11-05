#ifndef __VOFA_H
#define __VOFA_H

#include "stm32f10x.h"

/**
  * @brief  使用JustFloat协议发送浮点数数组
  * @param  data  指向要发送的浮点数数组
  * @param  count 要发送的浮点数个数
  * @retval None
  */
void VOFA_JustFloat_Send(float *data, uint8_t count);

#endif
