# Easy PDK Programmer Firmware - Porting Guide

This guide explains how to port the Easy PDK Programmer firmware to different microcontroller platforms.

## Architecture Overview

The firmware uses a **Board Abstraction Layer (BAL)** to isolate hardware-specific code:

```
┌─────────────────────────────────────────────┐
│         Application Layer                    │
│  (fpdk.c, fpdkusb.c, fpdkuart.c)            │
├─────────────────────────────────────────────┤
│      Board Abstraction Layer                │
│         (fpdk_board.h)                       │
├─────────────────────────────────────────────┤
│   Hardware Specific Implementation          │
│   (STM32 HAL, Custom MCU API, etc.)         │
└─────────────────────────────────────────────┘
```

## Files Structure

- **`fpdk.h`** - Public API declarations
- **`fpdk.c`** - Core IC programming logic (mostly platform-independent)
- **`fpdk_board.h`** - Board Abstraction Layer (platform-specific)
- **`fpdkusb.c`** - USB communication protocol
- **`fpdkuart.c`** - UART debug output

## Quick Start: Porting to a New Platform

### Step 1: Edit `fpdk_board.h`

1. Comment out the STM32 board definition:
```c
// #define FPDK_BOARD_STM32F072    1
```

2. Uncomment and rename the custom board section:
```c
#define FPDK_BOARD_MY_CUSTOM_MCU    1
```

### Step 2: Implement Required Macros

You must implement these hardware-specific macros:

#### GPIO Control Macros
```c
#define FPDK_CLK2_UP()              // Set CLK2 pin HIGH
#define FPDK_CLK2_DOWN()            // Set CLK2 pin LOW
#define FPDK_CLK_UP()               // Set CLK pin HIGH
#define FPDK_CLK_DOWN()             // Set CLK pin LOW
#define FPDK_SET_DAT_O(bit)         // Set DAT_O pin to bit value
#define FPDK_SET_DAT_F(bit)         // Set DAT_F pin to bit value
#define FPDK_GET_DAT()              // Read DAT pin (returns 0 or 1)
#define FPDK_SET_CMT(bit)           // Set CMT pin to bit value
```

#### GPIO Configuration Functions
```c
static inline void FPDK_GPIO_SetMode_Output(void* port, uint16_t pin);
static inline void FPDK_GPIO_SetMode_Input(void* port, uint16_t pin);
static inline void FPDK_GPIO_Write(void* port, uint16_t pin, bool state);
static inline bool FPDK_GPIO_Read(void* port, uint16_t pin);
```

#### Timing Functions
```c
static inline uint32_t FPDK_GetTick(void);      // Returns ms since boot
static inline void FPDK_DelayUS(uint32_t us);   // Microsecond delay
```

#### Board Configuration
```c
#define FPDK_VDD_DAC_MAX_MV         6260    // Max VDD voltage in mV
#define FPDK_VPP_DAC_MAX_MV         13270   // Max VPP voltage in mV
#define FPDK_VREFINT_CAL            1500    // ADC internal reference
#define FPDK_VDD_VALUE              3300    // Board VDD in mV
```

### Step 3: Implement Peripheral Support

The application layer (`fpdk.c`) still uses these STM32 HAL handles:

```c
extern ADC_HandleTypeDef  hadc;
extern DAC_HandleTypeDef  hdac;
extern TIM_HandleTypeDef  htim1, htim2, htim15;
extern SPI_HandleTypeDef  hspi1;
extern UART_HandleTypeDef huart1;
```

You have two options:

**Option A: HAL Compatibility Layer**
- Create stub structures matching STM32 HAL types
- Implement HAL-compatible functions

**Option B: Refactor Core Code**
- Replace HAL calls with your own peripheral API
- This requires more extensive changes to `fpdk.c`

### Step 4: USB Support

For USB communication (`fpdkusb.c`), you need:
- USB CDC (Virtual COM Port) implementation
- Functions: `CDC_IsHostPortOpen()`, `CDC_Transmit_FS()`

## Platform-Specific Requirements

### Minimum Hardware Requirements

| Resource | Requirement | Purpose |
|----------|-------------|---------|
| Flash | 32KB+ | Firmware code |
| RAM | 8KB+ | Buffers and stack |
| ADC | 12-bit, 3 channels | Voltage monitoring |
| DAC | 12-bit, 2 channels | VDD/VPP generation |
| Timers | 3+ | ADC trigger, calibration, PWM |
| SPI | 1x | Calibration measurement |
| UART | 1x | Debug output |
| USB | Full-speed | Communication |
| GPIO | 10+ | IC programming interface |
| Clock | 48MHz+ | USB and timing accuracy |

### Pin Mapping

You must map these logical pins to your MCU:

| Signal | Direction | Purpose |
|--------|-----------|---------|
| CLK | Output | Main programming clock |
| CLK2 | Output | Secondary clock (OTP3_1) |
| DAT | Bidirectional | Data line |
| DAT_O | Output | Data out (OTP types) |
| DAT_F | Output | Data out (FLASH types) |
| CMT | Output | Commit signal (OTP3_1) |
| PA0, PA4, PA7 | Bidirectional | Additional OTP3 signals |

## Example: ESP32 Port (Conceptual)

```c
#ifdef FPDK_BOARD_ESP32

#include <driver/gpio.h>
#include <esp_timer.h>

// Pin definitions
#define PIN_CLK     GPIO_NUM_2
#define PIN_CLK2    GPIO_NUM_4
#define PIN_DAT     GPIO_NUM_5
// ... more pins

// GPIO macros
#define FPDK_CLK_UP()    gpio_set_level(PIN_CLK, 1)
#define FPDK_CLK_DOWN()  gpio_set_level(PIN_CLK, 0)
#define FPDK_GET_DAT()   gpio_get_level(PIN_DAT)

// Timing
static inline uint32_t FPDK_GetTick(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static inline void FPDK_DelayUS(uint32_t us) {
    esp_rom_delay_us(us);
}

// Board config
#define FPDK_VDD_VALUE   3300
// ... more config

#endif // FPDK_BOARD_ESP32
```

## Example: WCH CH32X033/X035 Port (RISC-V)

The CH32X033 and CH32X035 are low-cost RISC-V microcontrollers from WCH with USB support. A reference implementation is included in `fpdk_board.h`.

### Hardware Capabilities

| Feature | CH32X033 | CH32X035 |
|---------|----------|----------|
| Core | QingKe RISC-V4C @ 48MHz | QingKe RISC-V4C @ 48MHz |
| Flash | 62KB | 48KB |
| SRAM | 20KB | 20KB |
| ADC | 12-bit, 14 channels | 12-bit, 14 channels |
| **DAC** | **No (use PWM)** | **Yes (hardware DAC)** |
| USB | 2.0 Full-Speed Device | 2.0 Full-Speed Device |
| USB PD | No | Yes |
| Timers | Advanced + General | Advanced + General |
| OPA/PGA | 2 sets | 2 sets |
| Price | ~$0.20 | ~$0.30 |

### Critical Hardware Requirements

**⚠️ IMPORTANT**: The CH32X033/35 cannot directly generate the high voltages needed for IC programming!

You **must** add external circuits:

1. **VPP Boost Converter** (5V → 14V)
   - Charge pump or boost converter
   - Controlled by DAC/PWM output
   - Voltage divider feedback to ADC

2. **VDD Regulator** (2V → 7V range)
   - Buck-boost converter or adjustable LDO
   - Controlled by DAC/PWM output
   - Voltage divider feedback to ADC

3. **DAC Implementation**
   - **CH32X035**: Use hardware DAC (recommended)
   - **CH32X033**: Use PWM + RC filter (R=6.8kΩ, C=470nF)

### Example Hardware Schematic

```
CH32X035 DAC_OUT ──┬─── [Op-Amp Buffer] ──── [Boost Converter] ──── VPP (14V max)
                   │                             │
                   │                             └─── [Voltage Divider] ──── ADC_IN
                   │
                   └─── [Op-Amp Buffer] ──── [Buck-Boost] ──── VDD (7V max)
                                               │
                                               └─── [Voltage Divider] ──── ADC_IN
```

### Pin Mapping Example

Modify these in `fpdk_board.h` for your board:

```c
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
```

### Using the Port

1. **Enable the board definition** in `fpdk_board.h`:
```c
// #define FPDK_BOARD_STM32F072    1
#define FPDK_BOARD_CH32X035    1   // Or CH32X033
```

2. **Install WCH peripheral library**:
```bash
# Download from: https://github.com/openwch/ch32x035
# Extract to: Firmware/source/Inc/
```

3. **Implement system tick** in your `main.c`:
```c
volatile uint32_t system_tick_ms = 0;

void SysTick_Handler(void) {
    system_tick_ms++;
}
```

4. **Initialize delay system** (if using WCH library):
```c
Delay_Init();  // Call in main() before using delays
```

5. **Calibrate timing**: The `FPDK_DelayUS()` uses cycle counting. Measure actual delays with oscilloscope and adjust the multiplier in `fpdk_board.h`:
```c
// Adjust '12' based on measurements
for(uint32_t i = 0; i < us * 12; i++) {
    __asm__ volatile ("nop");
}
```

### Known Limitations

1. **No factory-calibrated ADC reference** like STM32
   - You may need manual calibration

2. **CH32X033 lacks hardware DAC**
   - PWM approach adds noise
   - Consider external DAC chip (e.g., MCP4725)

3. **USB PD only on CH32X035**
   - Cannot use PD for self-powering on CH32X033

4. **External voltage circuits required**
   - Adds complexity and cost
   - Needs careful PCB design

### Recommended Configuration

For best results, use **CH32X035** with:
- Hardware DAC for clean voltage control
- TPS61230 boost converter for VPP
- TPS63000 buck-boost for VDD
- Precision voltage dividers (0.1% resistors)
- Decoupling capacitors near converters

### Development Tools

- **IDE**: MounRiver Studio (Eclipse-based)
- **Toolchain**: RISC-V GCC
- **Programmer**: WCH-Link
- **Debugger**: WCH-Link with GDB support

### Resources

- [CH32X035 GitHub](https://github.com/openwch/ch32x035)
- [WCH Official Site](https://www.wch-ic.com/products/CH32X035.html)
- [Datasheet PDF](https://www.wch-ic.com/downloads/CH32X035DS0_PDF.html)

---

## Example: Puya PY32F072 Port (ARM Cortex-M0+)

The PY32F072 is a low-cost STM32F072 alternative from Puya Semiconductor. It's nearly pin-compatible and uses a similar HAL structure.

### Hardware Capabilities

| Feature | PY32F072 | STM32F072 | Notes |
|---------|----------|-----------|-------|
| Core | Cortex-M0+ @ **72MHz** | Cortex-M0 @ 48MHz | **50% faster!** |
| Flash | **128KB** | 64-128KB | More memory |
| SRAM | **16KB** | 16KB | Same |
| ADC | 12-bit | 12-bit | Compatible |
| **DAC** | **2x 12-bit** ✓ | 2x 12-bit ✓ | **Perfect match!** |
| USB | 2.0 Full-Speed ✓ | 2.0 Full-Speed ✓ | Compatible |
| Timers | **13 timers** | 11 timers | More timers |
| OPA/CMP | **3+3** | 0 | Extra features! |
| CAN | **2.0** | 2.0 | Compatible |
| Price | **~$0.50** | ~$2-3 | **Much cheaper!** |

### Why Choose PY32F072?

**✅ PROS:**
- **Drop-in replacement** for STM32F072 hardware
- **Faster** CPU (72MHz vs 48MHz)
- **Cheaper** ($0.50 vs $2+)
- **Same HAL structure** - easy porting
- **2x hardware DAC** - perfect for VDD/VPP generation
- **More peripherals** (OPA, comparators)
- Uses existing hardware design (op-amps, boost circuits)

**⚠️ CONS:**
- Not 100% register-compatible (subtle differences)
- No LL (Low-Layer) library available
- Less mature ecosystem than STM32
- Factory calibration addresses may differ

### Porting Steps

The port is **95% compatible** with STM32F072 code:

1. **Replace STM32 HAL with PY32 HAL**:
```bash
# Remove STM32 HAL library
rm -rf Firmware/source/Inc/stm32f0xx_hal*

# Add PY32 HAL library (download from GitHub)
# https://github.com/IOsetting/py32f0-template
cp -r PY32F07x_HAL_Driver Firmware/source/Inc/
```

2. **Enable PY32F072 in fpdk_board.h**:
```c
// #define FPDK_BOARD_STM32F072    1
#define FPDK_BOARD_PY32F072    1
```

3. **Update include in main.c**:
```c
// Old: #include "stm32f0xx_hal.h"
#include "py32f0xx_hal.h"
```

4. **Verify calibration addresses**:
Check PY32F072 datasheet for factory calibration memory locations.

5. **Adjust timing** (already done in fpdk_board.h):
```c
// PY32F072 @ 72MHz needs 1.5x loop count vs STM32F072 @ 48MHz
asm volatile ("MOV R0,%[loops]\n1:\nSUB R0,#1\nCMP R0,#0\nBNE 1b"
              :: [loops]"r"(15*us) : "memory");
```

### Hardware Compatibility

The PY32F072 can use the **exact same PCB** as STM32F072:
- Same pinout (LQFP48/LQFP64)
- Same DAC outputs
- Same voltage reference
- Same USB pins
- Same ADC inputs

**Result:** Just swap the chip, update firmware, done!

### Development Tools

- **IDE**: Any ARM IDE (Keil, IAR, PlatformIO, VS Code)
- **Toolchain**: ARM GCC (arm-none-eabi)
- **Programmer**: J-Link, ST-Link (yes, ST-Link works!), PyOCD
- **Debugger**: Standard ARM SWD

### Resources

- [PY32F0 Template](https://github.com/IOsetting/py32f0-template)
- [Official Site](https://www.puyasemi.com/en/py32_series.html)
- [Datasheet PDF](https://download.py32.org/Datasheet/en/PY32F072_Datasheet_Rev1.1_EN.pdf)

---

## Example: WCH CH32V003 Port (RISC-V Ultra Low-Cost)

The CH32V003 is the **world's cheapest microcontroller** (~$0.10) with RISC-V core. It's challenging but possible!

### Hardware Capabilities

| Feature | CH32V003 | Challenge Level |
|---------|----------|----------------|
| Core | RISC-V RV32EC @ 48MHz | ⚠️ Minimal ISA |
| Flash | **16KB** | ⚠️ Tight fit |
| SRAM | **2KB** | 🔴 **CRITICAL** |
| ADC | **10-bit** (not 12-bit) | ⚠️ Lower resolution |
| **DAC** | **NONE** 🔴 | 🔴 **PWM required** |
| **USB** | **NONE** 🔴 | 🔴 **Software USB required** |
| Timers | Advanced + General ✓ | ✅ OK |
| OPA/CMP | 1 set ✓ | ✅ Helpful |
| GPIO | Up to 18 pins | ⚠️ Limited |
| Price | **~$0.10** | ✅ **Incredible!** |

### Critical Challenges

**🔴 Memory Crisis:**
- Only **2KB SRAM**! Original firmware uses ~8-10KB
- **SOLUTION**: Strip down to bare minimum
  - Remove debug strings
  - Reduce buffer sizes
  - Support only 1-2 IC types
  - Remove calibration feature

**🔴 No Hardware USB:**
- Must use [rv003usb](https://github.com/cnlohr/rv003usb) software USB
- USB Low-Speed only (1.5 Mbps)
- **Voltage limit: Max 3.6V** for reliable operation

**🔴 No Hardware DAC:**
- Must use PWM + RC filter
- 10-bit PWM resolution (1024 levels)
- More noise than hardware DAC

### External Circuits Required

```
USB D+/D- (bit-bang)
   ↓
CH32V003 ──→ PWM_VDD ──→ RC Filter ──→ Op-Amp ──→ Buck-Boost ──→ VDD (2-7V)
         ↓
         ──→ PWM_VPP ──→ RC Filter ──→ Op-Amp ──→ Boost Conv ──→ VPP (5-14V)
```

**Component count:** 2x RC filters, 2x Op-amps, 2x DC-DC converters, voltage dividers

### Is It Worth It?

**Honestly? Probably not for production.**

| Metric | CH32V003 | CH32X035 |
|--------|----------|----------|
| MCU Cost | $0.10 | $0.30 |
| External BOM | ~$2.00 | ~$1.00 |
| **Total** | **$2.10** | **$1.30** |
| Complexity | Very high | Low |
| Features | Minimal | Full |

**Verdict:** CH32X035 is cheaper AND better!

### When to Use CH32V003

- Learning RISC-V architecture
- Educational projects
- You enjoy challenges!

### Resources

- [CH32V003 GitHub](https://github.com/openwch/ch32v003)
- [rv003usb Library](https://github.com/cnlohr/rv003usb)
- [Datasheet PDF](https://www.wch-ic.com/downloads/CH32V003DS0_PDF.html)

---

## Timing Considerations

The IC programming protocol requires **precise microsecond timing**:

- Clock cycles: 1-2 µs
- Setup/hold times: < 1 µs
- Write pulses: 15-40 µs
- Erase pulses: 5000-40000 µs

Ensure your `FPDK_DelayUS()` implementation is accurate!

### Calibration Note

The calibration feature uses SPI slave mode to measure IC output frequency. This is optional but recommended for full functionality.

## Testing Your Port

1. **Compile** - Verify no compilation errors
2. **Basic I/O** - Test LED and button control commands
3. **Voltage Control** - Verify DAC output with multimeter
4. **IC Detection** - Try `PROBEIC` command with known IC
5. **Read/Write** - Test with supported PDK microcontroller

## Common Porting Issues

### Issue: Timing Inaccuracies
**Solution**: Use hardware timers or DWT cycle counter for delays

### Issue: USB Enumeration Fails
**Solution**: Check USB clock configuration (must be exactly 48MHz)

### Issue: Voltage Instability
**Solution**: Verify DAC reference voltage and op-amp configuration

### Issue: IC Not Detected
**Solution**: Check GPIO pin assignments and pull-up/down configuration

## Advanced: Removing STM32 Dependencies

To fully remove STM32 HAL dependencies from `fpdk.c`:

1. Create abstraction for ADC/DAC operations
2. Replace `HAL_GPIO_*` calls with `FPDK_GPIO_*`
3. Replace `HAL_GetTick()` with `FPDK_GetTick()`
4. Abstract timer and DMA operations
5. Remove STM32-specific peripheral handles

This is a larger refactoring effort but results in fully portable code.

## Resources

- [STM32 Reference Implementation](Firmware/source/Src/)
- [Original Hardware Design](https://free-pdk.github.io/)
- [Padauk IC Datasheets](https://free-pdk.github.io/)

## Contributing

If you successfully port to a new platform, please:
1. Add your board definition to `fpdk_board.h`
2. Document pin mappings and quirks
3. Submit a pull request!

## License

Same as main project: GPL-3.0

---

**Questions?** Open an issue on GitHub or ask on the Free-PDK forums.
