/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c 
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* 包含 ------------------------------------------------------------------*/
#include "stm32f10x_it.h"

/** @addtogroup STM32F10x_StdPeriph_Template
  * @{
  */

/* 私有类型 -----------------------------------------------------------*/
/* 私有定义 ------------------------------------------------------------*/
/* 私有宏 -------------------------------------------------------------*/
/* 私有变量 ---------------------------------------------------------*/
/* 私有函数原型 -----------------------------------------------*/
/* 私有函数 ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 处理器异常处理函数                         */
/******************************************************************************/

/**
  * @brief  处理NMI异常。
  * @param  无
  * @retval 无
  */
void NMI_Handler(void)
{
}

/**
  * @brief  处理硬件错误异常。
  * @param  无
  * @retval 无
  */
void HardFault_Handler(void)
{
  /* 硬件错误发生时进入死循环 */
  while (1)
  {
  }
}

/**
  * @brief  处理内存管理异常。
  * @param  无
  * @retval 无
  */
void MemManage_Handler(void)
{
  /* 内存管理异常发生时进入死循环 */
  while (1)
  {
  }
}

/**
  * @brief  处理总线错误异常。
  * @param  无
  * @retval 无
  */
void BusFault_Handler(void)
{
  /* 总线错误发生时进入死循环 */
  while (1)
  {
  }
}

/**
  * @brief  处理用法错误异常。
  * @param  无
  * @retval 无
  */
void UsageFault_Handler(void)
{
  /* 用法错误发生时进入死循环 */
  while (1)
  {
  }
}

/**
  * @brief  处理SVCall异常。
  * @param  无
  * @retval 无
  */
void SVC_Handler(void)
{
}

/**
  * @brief  处理调试监视器异常。
  * @param  无
  * @retval 无
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  处理PendSVC异常。
  * @param  无
  * @retval 无
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  处理SysTick中断。
  * @param  无
  * @retval 无
  */
void SysTick_Handler(void)
{
}

/******************************************************************************/
/*                 STM32F10x 外设中断处理函数                   */
/*  在此添加使用的外设中断处理函数（PPP），可用的外设中断处理函数名称请参阅启动 */
/*  文件（startup_stm32f10x_xx.s）。                                            */
/******************************************************************************/

/**
  * @brief  处理PPP中断请求。
  * @param  无
  * @retval 无
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
