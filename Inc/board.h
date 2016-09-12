#ifndef BOARD_H
#define BOARD_H

#include "stm32f4xx_hal.h"
#define LTE_VID	0x1546
#define LTE_PID 0x1146
typedef enum 
{
  LED3 = 0,//PORTF - GPS
  LED2 = 1,//PORTF - XsensINS
  LED1 = 2//PORTF - STINS
} Led_TypeDef;

#define LEDn     													3

#define LED3_PIN                         GPIO_PIN_8
#define LED3_GPIO_PORT                   GPIOF
#define LED3_GPIO_CLK_ENABLE()           __GPIOF_CLK_ENABLE()  
#define LED3_GPIO_CLK_DISABLE()          __GPIOF_CLK_DISABLE() 

#define LED2_PIN                         GPIO_PIN_9
#define LED2_GPIO_PORT                   GPIOF
#define LED2_GPIO_CLK_ENABLE()           __GPIOF_CLK_ENABLE()  
#define LED2_GPIO_CLK_DISABLE()          __GPIOF_CLK_DISABLE()  

#define LED1_PIN                         GPIO_PIN_10
#define LED1_GPIO_PORT                   GPIOF
#define LED1_GPIO_CLK_ENABLE()           __GPIOF_CLK_ENABLE()  
#define LED1_GPIO_CLK_DISABLE()          __GPIOF_CLK_DISABLE()  



#define LEDx_GPIO_CLK_ENABLE(__INDEX__) do{if((__INDEX__) == LED3) LED3_GPIO_CLK_ENABLE(); else \
                                           if((__INDEX__) == LED2) LED2_GPIO_CLK_ENABLE(); else \
                                           if((__INDEX__) == LED1) LED1_GPIO_CLK_ENABLE(); \
                                           }while(0)

#define LEDx_GPIO_CLK_DISABLE(__INDEX__) do{if((__INDEX__) == LED3) LED3_GPIO_CLK_DISABLE(); else \
                                            if((__INDEX__) == LED2) LED2_GPIO_CLK_DISABLE(); else \
                                            if((__INDEX__) == LED1) LED1_GPIO_CLK_DISABLE(); \
                                            }while(0)
																					

static GPIO_TypeDef* GPIO_PORT[LEDn] = { LED3_GPIO_PORT, 
                                 LED2_GPIO_PORT,
                                 LED1_GPIO_PORT};
static const uint16_t GPIO_PIN[LEDn] = {LED3_PIN, 
                                 LED2_PIN,
                                 LED1_PIN};

static inline void BSP_LED_Init(Led_TypeDef Led)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
  
  /* Enable the GPIO_LED Clock */
  LEDx_GPIO_CLK_ENABLE(Led);

  /* Configure the GPIO_LED pin */
  GPIO_InitStruct.Pin = GPIO_PIN[Led];
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
  
  HAL_GPIO_Init(GPIO_PORT[Led], &GPIO_InitStruct);
  
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET); 
}

static inline void BSP_LED_On(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_SET); 
}

/**
  * @brief  Turns selected LED Off.
  * @param  Led: Specifies the Led to be set off. 
  *   This parameter can be one of following parameters:
  *     @arg LED4
  *     @arg LED3
  *     @arg LED5
  *     @arg LED6 
  * @retval None
  */
static inline void  BSP_LED_Off(Led_TypeDef Led)
{
  HAL_GPIO_WritePin(GPIO_PORT[Led], GPIO_PIN[Led], GPIO_PIN_RESET); 
}

/**
  * @brief  Toggles the selected LED.
  * @param  Led: Specifies the Led to be toggled. 
  *   This parameter can be one of following parameters:
  *     @arg LED4
  *     @arg LED3
  *     @arg LED5
  *     @arg LED6  
  * @retval None
  */
static inline void  BSP_LED_Toggle(Led_TypeDef Led)
{
  HAL_GPIO_TogglePin(GPIO_PORT[Led], GPIO_PIN[Led]);
}

#endif //BOARD_H
