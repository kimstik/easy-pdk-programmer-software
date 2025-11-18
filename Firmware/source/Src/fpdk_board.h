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
// #define FPDK_BOARD_PY32F072   1
// #define FPDK_BOARD_CH32X033   1
// #define FPDK_BOARD_CH32X035   1
// #define FPDK_BOARD_CH32V003   1
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
// Puya PY32F072 Board Implementation (ARM Cortex-M0+)
// ============================================================================

#ifdef FPDK_BOARD_PY32F072

/*
 * Hardware Notes for PY32F072 Port:
 *
 * PY32F072 is a low-cost alternative to STM32F072 from Puya Semiconductor.
 * It's very similar to STM32F072 but NOT fully register-compatible!
 *
 * Key Features:
 *   - ARM Cortex-M0+ @ 72MHz (faster than STM32F072's 48MHz)
 *   - 128KB Flash, 16KB SRAM
 *   - 2x 12-bit DAC (perfect for VDD/VPP generation!)
 *   - 1x 12-bit ADC
 *   - USB 2.0 Full-Speed Device
 *   - 13 Timers, CAN 2.0, 3 OPA, 3 Comparators
 *   - Price: ~$0.50 (much cheaper than STM32)
 *
 * Register Differences from STM32:
 *   - Most peripherals are similar but with subtle register differences
 *   - HAL library available (PY32F07x_HAL_Driver)
 *   - No LL (Low-Layer) library available
 *   - Pin names and GPIO structure mostly compatible
 *
 * Voltage Generation:
 *   Same as STM32F072 - uses 2x DAC + op-amps for VDD/VPP generation
 *   Should work with existing hardware design!
 */

// Include PY32F072 HAL headers (similar to STM32 HAL)
#include "py32f0xx_hal.h"

// Board-specific max voltage values (same as STM32F072 design)
#define FPDK_VDD_DAC_MAX_MV         ( 6290 - 30)
#define FPDK_VPP_DAC_MAX_MV         (13300 - 30)

// PY32F072 chip-specific values
// Note: PY32 may not have factory calibration values in same location as STM32
// Check PY32F072 datasheet for actual calibration addresses
#define FPDK_TEMP030_CAL            ((uint32_t)*((uint16_t*)0x1FFFF7B8))  // Verify this address
#define FPDK_TEMP110_CAL            ((uint32_t)*((uint16_t*)0x1FFFF7C2))  // Verify this address
#define FPDK_VREFINT_CAL            ((uint32_t)*((uint16_t*)0x1FFFF7BA))  // Verify this address
#define FPDK_VDD_VALUE              3300

// GPIO macros for programming IO
// Assumes same pin assignments as STM32F072 board
// Adjust pin names if your PY32F072 board has different layout
#define FPDK_CLK2_UP()              HAL_GPIO_WritePin( IC_IO_PA0_UART1_TX_GPIO_Port, IC_IO_PA0_UART1_TX_Pin, GPIO_PIN_SET )
#define FPDK_CLK2_DOWN()            HAL_GPIO_WritePin( IC_IO_PA0_UART1_TX_GPIO_Port, IC_IO_PA0_UART1_TX_Pin, GPIO_PIN_RESET )
#define FPDK_CLK_UP()               HAL_GPIO_WritePin( IC_IO_PA3_CLK_GPIO_Port, IC_IO_PA3_CLK_Pin, GPIO_PIN_SET )
#define FPDK_CLK_DOWN()             HAL_GPIO_WritePin( IC_IO_PA3_CLK_GPIO_Port, IC_IO_PA3_CLK_Pin, GPIO_PIN_RESET )
#define FPDK_SET_DAT_O(bit)         HAL_GPIO_WritePin( IC_IO_PA4_GPIO_Port, IC_IO_PA4_Pin, bit?GPIO_PIN_SET:GPIO_PIN_RESET )
#define FPDK_SET_DAT_F(bit)         HAL_GPIO_WritePin( IC_IO_PA6_DAT_GPIO_Port, IC_IO_PA6_DAT_Pin, bit?GPIO_PIN_SET:GPIO_PIN_RESET )
#define FPDK_GET_DAT()              HAL_GPIO_ReadPin( IC_IO_PA6_DAT_GPIO_Port, IC_IO_PA6_DAT_Pin )
#define FPDK_SET_CMT(bit)           HAL_GPIO_WritePin( IC_IO_PA7_USART1_RX_GPIO_Port, IC_IO_PA7_USART1_RX_Pin, bit?GPIO_PIN_SET:GPIO_PIN_RESET )

// GPIO configuration functions (same as STM32 HAL)
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

// Microsecond delay using inline assembly for ARM Cortex-M0+
// At 72MHz, need to adjust loop count compared to STM32F072 @ 48MHz
static inline void FPDK_DelayUS(uint32_t us) {
  // At 72MHz vs 48MHz: multiply by 72/48 = 1.5x
  // Original was 10*us, now use 15*us
  asm volatile ("MOV R0,%[loops]\n1:\nSUB R0,#1\nCMP R0,#0\nBNE 1b"
                :: [loops]"r"(15*us) : "memory");
}

#endif // FPDK_BOARD_PY32F072

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
// WCH CH32V003 Board Implementation (RISC-V Ultra Low-Cost)
// ============================================================================

#ifdef FPDK_BOARD_CH32V003

/*
 * Hardware Notes for CH32V003 Port:
 *
 * CH32V003 is an ULTRA low-cost RISC-V microcontroller (~$0.10!)
 * BUT it has severe limitations for this application:
 *
 * CHALLENGES:
 *   - Only 2KB SRAM! (may not be enough for full firmware)
 *   - Only 16KB Flash (tight fit)
 *   - No hardware USB (must use rv003usb software USB)
 *   - No hardware DAC (must use PWM + RC filter)
 *   - Only 10-bit ADC (vs 12-bit on other platforms)
 *   - QingKe RISC-V2A core @ 48MHz (RV32EC - minimal instruction set)
 *
 * SOLUTIONS:
 *   1. Software USB: Use rv003usb library by cnlohr
 *      https://github.com/cnlohr/rv003usb
 *      - Implements USB Low-Speed device in firmware
 *      - Uses pin-change interrupts + bit-banging
 *      - Voltage limit: Max 3.6V for reliable USB operation
 *
 *   2. PWM DAC: Use Timer PWM + RC filter for voltage generation
 *      - RC filter: R=6.8kΩ, C=470nF (same as CH32X033)
 *      - Requires external op-amps and boost circuits
 *      - 10-bit PWM resolution (1024 levels)
 *
 *   3. Memory optimization: Strip down firmware
 *      - Remove debug strings
 *      - Optimize buffer sizes
 *      - Consider subset of IC types only
 *
 * EXTERNAL CIRCUITS REQUIRED:
 *   - Boost converter for VPP (5V → 14V)
 *   - Buck-boost for VDD (2V → 7V range)
 *   - Op-amp buffers for PWM outputs
 *   - Voltage dividers for ADC feedback
 *   - RC filters for PWM smoothing
 *
 * WHY USE THIS?
 *   - Absolute minimum cost (~$0.10 + passives)
 *   - Learning exercise / proof of concept
 *   - Embedded in custom ASICs or disposable programmers
 *
 * RECOMMENDATION: Use CH32X035 instead for production!
 */

// Include CH32V003 peripheral library headers
#include "ch32v003.h"
#include "ch32v003_gpio.h"
#include "ch32v003_rcc.h"
#include "ch32v003_tim.h"
#include "ch32v003_adc.h"

// Pin definitions - Customize for your board
// CH32V003 has limited pins (up to 18 GPIO)
#define FPDK_PIN_CLK2_PORT      GPIOC
#define FPDK_PIN_CLK2           GPIO_Pin_0
#define FPDK_PIN_CLK_PORT       GPIOC
#define FPDK_PIN_CLK            GPIO_Pin_3
#define FPDK_PIN_DAT_PORT       GPIOC
#define FPDK_PIN_DAT            GPIO_Pin_6
#define FPDK_PIN_DAT_O_PORT     GPIOC
#define FPDK_PIN_DAT_O          GPIO_Pin_4
#define FPDK_PIN_CMT_PORT       GPIOC
#define FPDK_PIN_CMT            GPIO_Pin_7

// PWM pins for DAC emulation
#define FPDK_PIN_PWM_VDD_PORT   GPIOD
#define FPDK_PIN_PWM_VDD        GPIO_Pin_4  // TIM2_CH1
#define FPDK_PIN_PWM_VPP_PORT   GPIOD
#define FPDK_PIN_PWM_VPP        GPIO_Pin_3  // TIM2_CH2

// Board-specific voltage values (depends on external op-amp/boost design)
#define FPDK_VDD_DAC_MAX_MV         6260
#define FPDK_VPP_DAC_MAX_MV         13270

// CH32V003 chip-specific values
#define FPDK_VREFINT_CAL            1200    // Typical internal reference ~1.2V
#define FPDK_VDD_VALUE              3300    // Board VDD (must be ≤3.6V for USB!)

// GPIO macros for programming IO
#define FPDK_CLK2_UP()              GPIO_WriteBit(FPDK_PIN_CLK2_PORT, FPDK_PIN_CLK2, Bit_SET)
#define FPDK_CLK2_DOWN()            GPIO_WriteBit(FPDK_PIN_CLK2_PORT, FPDK_PIN_CLK2, Bit_RESET)
#define FPDK_CLK_UP()               GPIO_WriteBit(FPDK_PIN_CLK_PORT, FPDK_PIN_CLK, Bit_SET)
#define FPDK_CLK_DOWN()             GPIO_WriteBit(FPDK_PIN_CLK_PORT, FPDK_PIN_CLK, Bit_RESET)
#define FPDK_SET_DAT_O(bit)         GPIO_WriteBit(FPDK_PIN_DAT_O_PORT, FPDK_PIN_DAT_O, (bit) ? Bit_SET : Bit_RESET)
#define FPDK_SET_DAT_F(bit)         GPIO_WriteBit(FPDK_PIN_DAT_PORT, FPDK_PIN_DAT, (bit) ? Bit_SET : Bit_RESET)
#define FPDK_GET_DAT()              GPIO_ReadInputDataBit(FPDK_PIN_DAT_PORT, FPDK_PIN_DAT)
#define FPDK_SET_CMT(bit)           GPIO_WriteBit(FPDK_PIN_CMT_PORT, FPDK_PIN_CMT, (bit) ? Bit_SET : Bit_RESET)

// GPIO configuration functions
typedef struct {
  GPIO_TypeDef* Port;
  uint16_t Pin;
  GPIOMode_TypeDef Mode;
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
    .GPIO_Mode = GPIO_Mode_IPD,
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
  extern volatile uint32_t system_tick_ms;
  return system_tick_ms;
}

// Microsecond delay for RISC-V QingKe RV32EC @ 48MHz
// RV32EC is a minimal instruction set (only 16 registers)
static inline void FPDK_DelayUS(uint32_t us) {
  // At 48MHz: 48 cycles per microsecond
  // Simple loop: ~4-5 cycles per iteration
  // Use ~10-12 iterations per microsecond
  for(uint32_t i = 0; i < us * 11; i++) {
    __asm__ volatile ("nop");
  }
}

/*
 * PWM DAC Implementation Notes:
 *
 * You'll need to implement:
 * 1. PWM initialization (TIM2_CH1 for VDD, TIM2_CH2 for VPP)
 * 2. PWM duty cycle control functions
 * 3. Integration with existing DAC interface in fpdk.c
 *
 * Example PWM setup (add to your main.c):
 *
 * void FPDK_PWM_Init(void) {
 *   // Configure TIM2 for 10-bit PWM @ ~24kHz
 *   // PWM frequency = 48MHz / (1 * 1024) = 46.875 kHz
 *   // RC filter: R=6.8k, C=470nF → cutoff ~500Hz
 * }
 *
 * void FPDK_PWM_SetVDD(uint16_t value) {
 *   // Set TIM2_CH1 duty cycle (0-1023)
 *   TIM_SetCompare1(TIM2, value);
 * }
 */

/*
 * Software USB Integration Notes:
 *
 * Use rv003usb library:
 * https://github.com/cnlohr/rv003usb
 *
 * CRITICAL: CH32V003 must run at ≤3.6V for USB operation!
 * USB D+ on PC4, D- on PC5 (check your variant)
 *
 * Add to your project:
 * 1. Include rv003usb library files
 * 2. Configure USB endpoint for CDC (Virtual COM Port)
 * 3. Replace HAL USB calls with rv003usb API
 * 4. Handle USB interrupts in pin-change ISR
 */

#endif // FPDK_BOARD_CH32V003

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
