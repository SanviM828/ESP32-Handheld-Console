# ESP32 Handheld Tetris Console: Embedded Systems Study

**Status:** Ongoing (Dec 2025 – Present)  
**Tech Stack:** ESP32 (C++), FreeRTOS, FastLED, Digital Signal Processing (Audio), Power Electronics

## 1. Project Overview
This project is an engineering deep-dive into designing a handheld gaming console from first principles. While the functional goal was to replicate the classic Tetris game logic, the technical objective was to master **32-bit microcontroller architecture**, **low-level peripheral management**, and **power distribution networks**.

The system features a custom display driver that maps 2D game logic to a serial LED stream, zero-latency input handling via hardware polling, and a PWM-based audio synthesis engine. It demonstrates practical application of interrupt management, coordinate transformation matrices, and efficient memory usage on the ESP32 platform.

## 2. System Architecture

### 2.1 Hardware Overview
The core processing unit is the **ESP32 WROOM-32**, chosen for its dual-core architecture and high clock speed (240MHz), ensuring flicker-free display updates compared to 8-bit alternatives. 

Power is managed via an **LM2596 Buck Converter**, stepping down a 9V DC source to a stable 5V logic rail. The circuit utilizes a **"Split Power" topology** where the Buck Converter output feeds the LED Matrix and ESP32 `VIN` pin in parallel. This prevents the high LED current (up to 2A at full white brightness) from passing through the ESP32 PCB traces, effectively mitigating thermal risks and voltage sag on the logic rail.



### 2.2 Component Bill of Materials (BOM)
| Component | Specification | Function |
|-----------|---------------|----------|
| **Microcontroller** | ESP32 DevKit V1 (30-Pin) | Central processing and logic control |
| **Display** | 8x8 WS2812B Matrix | Addressable RGB display grid |
| **Audio** | TDK PS110 Passive Buzzer | PWM-driven sound synthesis |
| **Power Regulation** | LM2596 Buck Converter | 9V to 5V DC-DC Step-down |
| **Input Interface** | 5x Tactile Switches | Active-Low user inputs |
| **Power Source** | 9V Alkaline / Li-Ion | Primary power supply |

### 2.3 Component Selection Rationale
*Design decisions regarding power management and signal integrity.*

**1. Power Regulation: LM2596 vs. Linear Regulators (7805)**
* **Initial Concept:** L7805CV Linear Regulator.
* **Problem:** The LED matrix, when displaying full white, can draw significant current. Stepping down 9V to 5V creates a 4V drop. At moderate loads (e.g., 500mA), a linear regulator would dissipate 2 Watts of heat ($P = V_{drop} \times I$), requiring a bulky heatsink.
* **Selected Solution:** **LM2596 Buck Converter**.
    * **Reason:** It uses switching topology to achieve >80% efficiency, minimizing thermal loss and preserving battery life for portable operation.

**2. Audio: Passive vs. Active Buzzer**
* **Initial Concept:** SFM-27 Active Buzzer.
* **Problem:** Active buzzers oscillate at a fixed internal frequency when powered, making them incapable of playing melodies.
* **Selected Solution:** **TDK PS110 Passive Transducer**.
    * **Reason:** Allows the firmware to modulate the PWM frequency (1000Hz–2500Hz) to generate distinct musical notes for gameplay events (Line Clear, Tetris Theme, Game Over).

**3. Microcontroller: ESP32 vs. Arduino Nano**
* **Reason for Change:** The WS2812B LED protocol requires precise timing. The ESP32's higher clock speed (240MHz vs 16MHz) ensures that display refreshing does not block input polling.

## 3. Implementation Details

### 3.1 Pin Configuration & Planning
Careful pin planning was required to avoid ESP32 "Strapping Pins" (pins that determine boot modes) and UART conflicts.

| Function | GPIO Pin | Configuration | Notes |
|----------|----------|---------------|-------|
| **Button: Left** | GPIO 15 | INPUT_PULLUP | *Relocated from GPIO 2 (Strapping Pin conflict)* |
| **Button: Right** | GPIO 3 | INPUT_PULLUP | *Shared with UART0 RX; Disconnect during upload* |
| **Button: Rotate**| GPIO 4 | INPUT_PULLUP | - |
| **Button: Drop** | GPIO 14 | INPUT_PULLUP | - |
| **Button: Pause** | GPIO 18 | INPUT_PULLUP | - |
| **Audio Out** | GPIO 23 | OUTPUT | PWM signal generation |
| **Display Data** | GPIO 25 | OUTPUT | WS2812B Data Line |

### 3.2 Circuit Diagram
![Circuit Schematic](media/tetris_ckt)

## 4. Engineering Challenges & Resolutions
This section documents the critical failures encountered during development and the engineering principles applied to resolve them.

### Challenge 1: The "Strapping Pin" Conflict (GPIO 2)
**The Issue:** The "Left" movement button was initially mapped to GPIO 2. During testing, the button failed to register inputs reliably, and the board would sometimes fail to boot.
**Root Cause Analysis:** Consulting the datasheet revealed that GPIO 2 is a **Strapping Pin** used to determine boot mode. It also connects to the on-board Blue LED, which acts as a pull-down resistor, interfering with the internal `INPUT_PULLUP` logic required for the switch.
**The Solution:** Remapped the "Left" button to **GPIO 15**, a standard GPIO pin, resolving the conflict without external hardware changes.

### Challenge 2: UART Bus Contention
**The Issue:** The "Right" button was mapped to GPIO 3 (RX0). While gameplay worked, uploading new code failed with a "Time Out" error.
**Root Cause Analysis:** GPIO 3 is the Serial Receive (RX) line for the USB-to-UART bridge. Connecting a button created a path to ground that corrupted the data stream during flashing.
**The Solution:** Implemented a hardware workflow protocol: The GPIO 3 jumper wire is physically disconnected during the bootloader sequence and reconnected immediately after successful flashing.

### Challenge 3: Input "Ghosting" (EMI)
**The Issue:** The buzzer would emit random sounds and pieces would move without user interaction.
**Root Cause Analysis:** Input pins were left floating. Long jumper wires acted as antennas, coupling capacitive noise from the environment and triggering false digital interrupts.
**The Solution:**
1. **Software:** Enforced internal `INPUT_PULLUP` resistors on all input pins.
2. **Hardware:** Established a **"Star Ground"** topology at the Buck Converter output to minimize ground loop noise.

### Challenge 4: Matrix Orientation
**The Issue:** The physical mounting of the LED matrix resulted in the coordinate system being inverted (Tetrominos fell upwards).
**The Solution:** Implemented a coordinate transformation layer in the display driver (`getPixelIndex`). By calculating `newX = (WIDTH - 1) - x` and `newY = (HEIGHT - 1) - y`, the software logic remains standard while visually correcting the output 180 degrees.

## 5. Software Logic (The "Tetris Engine")
The firmware avoids blocking `delay()` calls to maintain responsiveness. It uses a **Polled State Machine** architecture:

* **Input State:** Scans GPIOs every 1ms with software debouncing (150ms threshold).
* **Logic State:** Calculates gravity/drop timers and handles collision detection (checking boundaries and board state).
* **Render State:** Maps the virtual board to the physical LED array using the coordinate transformation layer.

**Repository Structure:** The complete source code is available in the [`src/`](src/) directory.

## 6. Project Timeline & Learning Outcomes
**Duration:** Dec 2025 – Present

* **Phase 1 (Simulation):** Validated logic flow and array management.
* **Phase 2 (Breadboard Prototype):** Established power distribution and verified the ESP32/Level-Shifter logic.
* **Phase 3 (Optimization):** Migrated from blocking loops to asynchronous timer-based execution to allow for smooth audio multitasking.

## 7. Current Status & Future Roadmap
The project is currently in the **"Electronic Verification Complete"** stage. The core logic, power distribution, and drivers are fully functional on the breadboard prototype.

### Phase 4: Hardware Consolidation (Upcoming)
The next phase involves transitioning from the temporary breadboard to a permanent assembly.
* **Circuit Hardening:** Migrating the circuit to a **Perfboard (Dot PCB)** using point-to-point soldering to improve mechanical stability and vibration resistance.
* **Enclosure Design:** Finalizing a "Clamshell" style case in **Fusion 360** to house the electronics and battery.
* **Power Management:** Integrating a **TP4056** module to support rechargeable Li-Ion cells, replacing the disposable 9V battery.

## Disclaimer
This project is an educational exploration of embedded systems design. It is not an official product. All software is provided "as is" under the MIT License.
