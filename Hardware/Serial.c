#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdarg.h>

uint8_t Serial_RxData;		//定义串口接收的数据变量
uint8_t Serial_RxFlag;		//定义串口接收的标志位变量

/**
 * 函数名：Serial_Init
 * 功  能：初始化USART1串口（标准配置），波特率115200，使用固定引脚PA9(TX)/PA10(RX)
 * 用  途：与上位机通信，接收命令和发送数据
 * 硬件要求：PA9接外部设备RX，PA10接外部设备TX（必须交叉连接！）
 * 注意事项：STM32F103系列USART1引脚不可重映射，此为硬件固定配置
 */
void Serial_Init(void)
{
    /* 1. 开启外设时钟（先开GPIO再开USART）*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);  // 必须先开GPIO时钟（引脚配置依赖）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE); // 再开USART1时钟
    
    /* 2. GPIO引脚配置（严格遵循硬件映射）*/
    GPIO_InitTypeDef GPIO_InitStruct;
    
    // PA9: USART1_TX → 复用推挽输出（必须50MHz高速）
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;  // 复用推挽
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // 高速确保波形质量
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    // PA10: USART1_RX → 上拉输入（必须上拉！）
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING; // 浮空输入（更符合UART协议）
    // 替代方案：若环境干扰大，用GPIO_Mode_IPU（上拉输入）
    GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* 3. USART核心配置 */
    USART_InitTypeDef USART_InitStruct;
    USART_InitStruct.USART_BaudRate = 115200;             // 高速波特率
    USART_InitStruct.USART_WordLength = USART_WordLength_8b; // 8位数据
    USART_InitStruct.USART_StopBits = USART_StopBits_1;   // 1位停止位
    USART_InitStruct.USART_Parity = USART_Parity_No;      // 无校验
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无流控
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 全双工
    USART_Init(USART1, &USART_InitStruct);
    
    /* 4. 中断配置（可选，根据需求启用）*/
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // 使能接收中断
    
    /* 5. NVIC配置（中断优先级）*/
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 2位抢占+2位子优先级
    
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;       // USART1中断通道
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 2; // 抢占优先级2
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 2;      // 子优先级2
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;         // 使能通道
    NVIC_Init(&NVIC_InitStruct);
    
    /* 6. 最终使能 */
    USART_Cmd(USART1, ENABLE); // 必须最后使能！
}



/**
  * 函    数：串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1, Byte);		//将字节数据写入数据寄存器，写入后USART自动生成时序波形
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);	//等待发送完成
	/*下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位*/
}

/**
  * 函    数：串口发送一个数组
  * 参    数：Array 要发送数组的首地址
  * 参    数：Length 要发送数组的长度
  * 返 回 值：无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i ++)		//遍历数组
	{
		Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
  * 函    数：串口发送一个字符串
  * 参    数：String 要发送字符串的首地址
  * 返 回 值：无
  */
void Serial_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i ++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
	{
		Serial_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
	}
}

/**
  * 函    数：次方函数（内部使用）
  * 返 回 值：返回值等于X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;	//设置结果初值为1
	while (Y --)			//执行Y次
	{
		Result *= X;		//将X累乘到结果
	}
	return Result;
}

/**
  * 函    数：串口发送数字
  * 参    数：Number 要发送的数字，范围：0~4294967295
  * 参    数：Length 要发送数字的长度，范围：0~10
  * 返 回 值：无
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i ++)		//根据数字长度遍历数字的每一位
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//依次调用Serial_SendByte发送每位数字
	}
}

/**
  * 函    数：使用printf需要重定向的底层函数
  * 参    数：保持原始格式即可，无需变动
  * 返 回 值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
	Serial_SendByte(ch);			//将printf的底层重定向到自己的发送字节函数
	return ch;
}

/**
  * 函    数：自己封装的prinf函数
  * 参    数：format 格式化字符串
  * 参    数：... 可变的参数列表
  * 返 回 值：无
  */
void Serial_Printf(char *format, ...)
{
	char String[100];				//定义字符数组
	va_list arg;					//定义可变参数列表数据类型的变量arg
	va_start(arg, format);			//从format开始，接收参数列表到arg变量
	vsprintf(String, format, arg);	//使用vsprintf打印格式化字符串和参数列表到字符数组中
	va_end(arg);					//结束变量arg
	Serial_SendString(String);		//串口发送字符数组（字符串）
}

/**
  * 函    数：获取串口接收标志位
  * 参    数：无
  * 返 回 值：串口接收标志位，范围：0~1，接收到数据后，标志位置1，读取后标志位自动清零
  */
uint8_t Serial_GetRxFlag(void)
{
	if (Serial_RxFlag == 1)			//如果标志位为1
	{
		Serial_RxFlag = 0;
		return 1;					//则返回1，并自动清零标志位
	}
	return 0;						//如果标志位为0，则返回0
}

/**
  * 函    数：获取串口接收的数据
  * 参    数：无
  * 返 回 值：接收的数据，范围：0~255
  */
uint8_t Serial_GetRxData(void)
{
	return Serial_RxData;			//返回接收的数据变量
}
