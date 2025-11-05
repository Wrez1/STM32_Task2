#include "vofa.h"

/**
  * @brief  使用JustFloat协议发送浮点数数组
  * @param  data  指向要发送的浮点数数组
  * @param  count 要发送的浮点数个数
  * @retval None
  */
void VOFA_JustFloat_Send(float *data, uint8_t count)
{
    uint8_t i;
    uint8_t *byte_ptr;
    
    // 1. 发送浮点数数据 (将float数组视为字节数组发送)
    byte_ptr = (uint8_t *)data;
    for (i = 0; i < count * 4; i++)
    {
        USART_SendData(USART1, *byte_ptr);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        byte_ptr++;
    }
    
    // 2. 发送JustFloat协议的固定帧尾
    const uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7f};
    for (i = 0; i < 4; i++)
    {
        USART_SendData(USART1, tail[i]);
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    }
}
