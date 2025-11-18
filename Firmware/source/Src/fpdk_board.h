/*
Copyright (C) 2019-2020  freepdk  https://free-pdk.github.io

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __FPDK_BOARD_H_
#define __FPDK_BOARD_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * Board abstraction layer for FPDK programmer
 * This file isolates hardware-specific code for easy porting
 */

// ============================================================================
// Board Configuration
// ============================================================================

// Select board type (uncomment one):
#define FPDK_BOARD_STM32F072    1
// #define FPDK_BOARD_CUSTOM    1

// ============================================================================
// STM32F072 Board Implementation
// ============================================================================

#ifdef FPDK_BOARD_STM32F072

#include "main.h"

// Board-specific max voltage values (DAC max => mV max after opamp output / -30 mV DAC DC offset)
#define FPDK_VDD_DAC_MAX_MV         ( 6290 - 30)
#define FPDK_VPP_DAC_MAX_MV         (13300 - 30)

// STM32F072 chip-specific factory calibration values in ROM
#define FPDK_TEMP030_CAL            ((uint32_t)*((uint16_t*)0x1FFFF7B8))
#define FPDK_TEMP110_CAL            ((uint32_t)*((uint16_t*)0x1FFFF7C2))
#define FPDK_VREFINT_CAL            ((uint32_t)*((uint16_t*)0x1FFFF7BA))
#define FPDK_VDD_VALUE              3300

// GPIO macros for programming IO
#define FPDK_CLK2_UP()              HAL_GPIO_WritePin( IC_IO_PA0_UART1_TX_GPIO_Port, IC_IO_PA0_UART1_TX_Pin, GPIO_PIN_SET )
#define FPDK_CLK2_DOWN()            HAL_GPIO_WritePin( IC_IO_PA0_UART1_TX_GPIO_Port, IC_IO_PA0_UART1_TX_Pin, GPIO_PIN_RESET )
#define FPDK_CLK_UP()               HAL_GPIO_WritePin( IC_IO_PA3_CLK_GPIO_Port, IC_IO_PA3_CLK_Pin, GPIO_PIN_SET )
#define FPDK_CLK_DOWN()             HAL_GPIO_WritePin( IC_IO_PA3_CLK_GPIO_Port, IC_IO_PA3_CLK_Pin, GPIO_PIN_RESET )
#define FPDK_SET_DAT_O(bit)         HAL_GPIO_WritePin( IC_IO_PA4_GPIO_Port, IC_IO_PA4_Pin, bit?GPIO_PIN_SET:GPIO_PIN_RESET )
#define FPDK_SET_DAT_F(bit)         HAL_GPIO_WritePin( IC_IO_PA6_DAT_GPIO_Port, IC_IO_PA6_DAT_Pin, bit?GPIO_PIN_SET:GPIO_PIN_RESET )
#define FPDK_GET_DAT()              HAL_GPIO_ReadPin( IC_IO_PA6_DAT_GPIO_Port, IC_IO_PA6_DAT_Pin )
#define FPDK_SET_CMT(bit)           HAL_GPIO_WritePin( IC_IO_PA7_USART1_RX_GPIO_Port, IC_IO_PA7_USART1_RX_Pin, bit?GPIO_PIN_SET:GPIO_PIN_RESET )

// GPIO configuration functions
typedef struct {
  void* Port;
  uint16_t Pin;
  uint32_t Mode;
  uint32_t Pull;
  uint32_t Speed;
} FPDK_GPIO_InitTypeDef;

static inline void FPDK_GPIO_SetMode_Output(void* port, uint16_t pin) {
  HAL_GPIO_WritePin((GPIO_TypeDef*)port, pin, GPIO_PIN_RESET);
  GPIO_InitTypeDef init = { .Pin = pin, .Mode = GPIO_MODE_OUTPUT_PP, .Speed = GPIO_SPEED_FREQ_HIGH };
  HAL_GPIO_Init((GPIO_TypeDef*)port, &init);
}

static inline void FPDK_GPIO_SetMode_Input(void* port, uint16_t pin) {
  GPIO_InitTypeDef init = { .Pin = pin, .Mode = GPIO_MODE_INPUT, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_HIGH };
  HAL_GPIO_Init((GPIO_TypeDef*)port, &init);
}

static inline void FPDK_GPIO_Write(void* port, uint16_t pin, bool state) {
  HAL_GPIO_WritePin((GPIO_TypeDef*)port, pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static inline bool FPDK_GPIO_Read(void* port, uint16_t pin) {
  return HAL_GPIO_ReadPin((GPIO_TypeDef*)port, pin) != GPIO_PIN_RESET;
}

// Timing functions
static inline uint32_t FPDK_GetTick(void) {
  return HAL_GetTick();
}

// Microsecond delay using inline assembly for ARM Cortex-M0
static inline void FPDK_DelayUS(uint32_t us) {
  asm volatile ("MOV R0,%[loops]\n1:\nSUB R0,#1\nCMP R0,#0\nBNE 1b"
                :: [loops]"r"(10*us) : "memory");
}

#endif // FPDK_BOARD_STM32F072

// ============================================================================
// Custom Board Template (for porting)
// ============================================================================

#ifdef FPDK_BOARD_CUSTOM

// TODO: Define your board-specific values here
#define FPDK_VDD_DAC_MAX_MV         6260
#define FPDK_VPP_DAC_MAX_MV         13270
#define FPDK_VREFINT_CAL            1500
#define FPDK_VDD_VALUE              3300

// TODO: Implement GPIO macros for your platform
#define FPDK_CLK2_UP()              // Your implementation
#define FPDK_CLK2_DOWN()            // Your implementation
#define FPDK_CLK_UP()               // Your implementation
#define FPDK_CLK_DOWN()             // Your implementation
#define FPDK_SET_DAT_O(bit)         // Your implementation
#define FPDK_SET_DAT_F(bit)         // Your implementation
#define FPDK_GET_DAT()              // Your implementation
#define FPDK_SET_CMT(bit)           // Your implementation

// TODO: Implement GPIO configuration functions
static inline void FPDK_GPIO_SetMode_Output(void* port, uint16_t pin) {
  // Your implementation
}

static inline void FPDK_GPIO_SetMode_Input(void* port, uint16_t pin) {
  // Your implementation
}

static inline void FPDK_GPIO_Write(void* port, uint16_t pin, bool state) {
  // Your implementation
}

static inline bool FPDK_GPIO_Read(void* port, uint16_t pin) {
  // Your implementation
  return false;
}

// TODO: Implement timing functions
static inline uint32_t FPDK_GetTick(void) {
  // Your implementation - return milliseconds since startup
  return 0;
}

static inline void FPDK_DelayUS(uint32_t us) {
  // Your implementation - microsecond delay
}

#endif // FPDK_BOARD_CUSTOM

// ============================================================================
// Common Macros (Board Independent)
// ============================================================================

// General macros for programming IO
#define FPDK_Clock()                { FPDK_CLK_UP(); FPDK_DelayUS(1); FPDK_CLK_DOWN(); }
#define FPDK_Clock2()               { FPDK_CLK2_UP(); FPDK_DelayUS(1); FPDK_CLK2_DOWN(); }
#define FPDK_Commit2()              { FPDK_SET_CMT(1); FPDK_DelayUS(1); FPDK_SET_CMT(0); FPDK_Clock2(); }
#define FPDK_SendBitO(bit)          { FPDK_SET_DAT_O(bit); FPDK_Clock(); }
#define FPDK_SendBitO2(bit)         { FPDK_SET_DAT_O(bit); FPDK_Clock2(); }
#define FPDK_SendBitF(bit)          { FPDK_SET_DAT_F(bit); FPDK_Clock(); }
#define FPDK_RecvBit()              ({ FPDK_CLK_UP(); FPDK_DelayUS(1); uint32_t bit=FPDK_GET_DAT(); FPDK_CLK_DOWN(); bit; })
#define FPDK_RecvBit2()             ({ FPDK_CLK2_UP(); FPDK_DelayUS(1); uint32_t bit=FPDK_GET_DAT(); FPDK_CLK2_DOWN(); bit; })

// PDK command timings (board independent)
#define FPDK_VPP_CMD_STABELIZE_DELAYUS      100
#define FPDK_VDD_CMD_STABELIZE_DELAYUS      500
#define FPDK_VPP_R_STABELIZE_DELAYUS        1000
#define FPDK_VDD_R_STABELIZE_DELAYUS        1000
#define FPDK_VPP_EW_STABELIZE_DELAYUS       10000
#define FPDK_VDD_EW_STABELIZE_DELAYUS       10000
#define FPDK_VDD_STOP_DELAYUS               0
#define FPDK_VPP_STOP_DELAYUS               0
#define FPDK_LEAVE_PROG_MODE_DELAYUS        10000
#define FPDK_VDD_CAL_STARTUP_DELAYUS        1000

#endif //__FPDK_BOARD_H_
