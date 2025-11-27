/* Minimal STM32F10x config */
#ifndef __STM32F10x_CONF_H
#define __STM32F10x_CONF_H

// Only include what we need
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"

// Assert disabled for minimal build
#define assert_param(expr) ((void)0)

#endif
