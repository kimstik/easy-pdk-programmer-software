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
// #define FPDK_BOARD_CH32X033   1
// #define FPDK_BOARD_CH32X035   1
// #define FPDK_BOARD_CUSTOM     1

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
// WCH CH32X033/CH32X035 Board Implementation (RISC-V)
// ============================================================================

#if defined(FPDK_BOARD_CH32X033) || defined(FPDK_BOARD_CH32X035)

/*
 * Hardware Notes for CH32X033/35 Port:
 *
 * CRITICAL REQUIREMENT: External voltage boost circuits needed!
 * The Easy PDK Programmer requires:
 *   - VDD: 2.0V to 6.26V (adjustable)
 *   - VPP: 5.0V to 13.27V (adjustable)
 *
 * Since CH32X033/35 operates at 3.3V or 5V, you must add:
 *   1. Boost converter or charge pump for VPP (up to 14V)
 *   2. Buck-boost or adjustable regulator for VDD (2-7V range)
 *   3. Voltage divider feedback to ADC for monitoring
 *
 * DAC Options:
 *   - CH32X035: Has hardware DAC (recommended)
 *   - CH32X033: Use PWM + RC filter or external DAC chip
 *
 * MCU Capabilities:
 *   - QingKe RISC-V4C core @ 48MHz (RV32IMAC)
 *   - CH32X033: 62KB Flash, 20KB SRAM
 *   - CH32X035: 48KB Flash, 20KB SRAM
 *   - 12-bit ADC, TouchKey, OPA/PGA, Comparators
 *   - USB 2.0 Full-Speed Device
 *   - Advanced Timers with PWM support
 */

// Include CH32X033/35 peripheral library headers
#include "ch32x035.h"           // Main device header
#include "ch32x035_gpio.h"      // GPIO functions
#include "ch32x035_rcc.h"       // Clock control
#include "ch32x035_tim.h"       // Timer functions
#include "ch32x035_adc.h"       // ADC functions
#ifdef FPDK_BOARD_CH32X035
#include "ch32x035_dac.h"       // DAC (CH32X035 only)
#endif

// Pin definitions - Customize these for your board layout
// These are example pin assignments - adjust for your hardware!
#define FPDK_PIN_CLK2_PORT      GPIOA
#define FPDK_PIN_CLK2           GPIO_Pin_0
#define FPDK_PIN_CLK_PORT       GPIOA
#define FPDK_PIN_CLK            GPIO_Pin_3
#define FPDK_PIN_DAT_PORT       GPIOA
#define FPDK_PIN_DAT            GPIO_Pin_6
#define FPDK_PIN_DAT_O_PORT     GPIOA
#define FPDK_PIN_DAT_O          GPIO_Pin_4
#define FPDK_PIN_CMT_PORT       GPIOA
#define FPDK_PIN_CMT            GPIO_Pin_7

// Board-specific voltage values (depends on external boost circuit design)
// These values assume external op-amps with similar gain to STM32F072 board
#define FPDK_VDD_DAC_MAX_MV         6260
#define FPDK_VPP_DAC_MAX_MV         13270

// CH32X033/35 chip-specific values
#define FPDK_VREFINT_CAL            1500    // Typical internal reference (check datasheet)
#define FPDK_VDD_VALUE              3300    // Board VDD in mV (3.3V or 5.0V)

// GPIO macros for programming IO
#define FPDK_CLK2_UP()              GPIO_SetBits(FPDK_PIN_CLK2_PORT, FPDK_PIN_CLK2)
#define FPDK_CLK2_DOWN()            GPIO_ResetBits(FPDK_PIN_CLK2_PORT, FPDK_PIN_CLK2)
#define FPDK_CLK_UP()               GPIO_SetBits(FPDK_PIN_CLK_PORT, FPDK_PIN_CLK)
#define FPDK_CLK_DOWN()             GPIO_ResetBits(FPDK_PIN_CLK_PORT, FPDK_PIN_CLK)
#define FPDK_SET_DAT_O(bit)         GPIO_WriteBit(FPDK_PIN_DAT_O_PORT, FPDK_PIN_DAT_O, (bit) ? Bit_SET : Bit_RESET)
#define FPDK_SET_DAT_F(bit)         GPIO_WriteBit(FPDK_PIN_DAT_PORT, FPDK_PIN_DAT, (bit) ? Bit_SET : Bit_RESET)
#define FPDK_GET_DAT()              GPIO_ReadInputDataBit(FPDK_PIN_DAT_PORT, FPDK_PIN_DAT)
#define FPDK_SET_CMT(bit)           GPIO_WriteBit(FPDK_PIN_CMT_PORT, FPDK_PIN_CMT, (bit) ? Bit_SET : Bit_RESET)

// GPIO configuration functions
typedef struct {
  GPIO_TypeDef* Port;
  uint16_t Pin;
  GPIOMode_TypeDef Mode;
  GPIOSpeed_TypeDef Speed;
} FPDK_GPIO_InitTypeDef;

static inline void FPDK_GPIO_SetMode_Output(void* port, uint16_t pin) {
  GPIO_WriteBit((GPIO_TypeDef*)port, pin, Bit_RESET);
  GPIO_InitTypeDef init = {
    .GPIO_Pin = pin,
    .GPIO_Mode = GPIO_Mode_Out_PP,
    .GPIO_Speed = GPIO_Speed_50MHz
  };
  GPIO_Init((GPIO_TypeDef*)port, &init);
}

static inline void FPDK_GPIO_SetMode_Input(void* port, uint16_t pin) {
  GPIO_InitTypeDef init = {
    .GPIO_Pin = pin,
    .GPIO_Mode = GPIO_Mode_IPD,  // Input with pull-down
    .GPIO_Speed = GPIO_Speed_50MHz
  };
  GPIO_Init((GPIO_TypeDef*)port, &init);
}

static inline void FPDK_GPIO_Write(void* port, uint16_t pin, bool state) {
  GPIO_WriteBit((GPIO_TypeDef*)port, pin, state ? Bit_SET : Bit_RESET);
}

static inline bool FPDK_GPIO_Read(void* port, uint16_t pin) {
  return GPIO_ReadInputDataBit((GPIO_TypeDef*)port, pin) != Bit_RESET;
}

// Timing functions
static inline uint32_t FPDK_GetTick(void) {
  // Assumes you have a SysTick or timer-based millisecond counter
  // Implement this based on your system tick configuration
  extern volatile uint32_t system_tick_ms;  // Define this in your main.c
  return system_tick_ms;
}

// Microsecond delay for RISC-V QingKe core @ 48MHz
// This is a simple cycle-counting delay - adjust loops for your clock speed
static inline void FPDK_DelayUS(uint32_t us) {
  // At 48MHz: 48 cycles per microsecond
  // Assuming ~4 cycles per loop iteration: 12 loops per microsecond
  // Adjust this multiplier based on actual timing measurements
  for(uint32_t i = 0; i < us * 12; i++) {
    __asm__ volatile ("nop");
  }
}

/*
 * Alternative: Use WCH peripheral library delay functions
 * If you've initialized the delay system with Delay_Init():
 *
 * static inline void FPDK_DelayUS(uint32_t us) {
 *   Delay_Us(us);
 * }
 */

#endif // FPDK_BOARD_CH32X033 || FPDK_BOARD_CH32X035

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
