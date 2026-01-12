# ESP32 Handheld Tetris Console

## 1. Introduction
This project details the design and implementation of a handheld gaming console powered by the ESP32 microcontroller. The system emulates the classic Tetris game logic, featuring a custom-built 8x8 RGB LED matrix display, low-latency input handling, and hardware-PWM audio synthesis. 

The project focuses on embedded systems concepts including GPIO architecture, interrupt-based input handling, power distribution for mixed-voltage components, and efficient memory management for display buffering.

## 2. System Architecture

### 2.1 Hardware Overview
The core processing unit is the ESP32 WROOM-32, chosen for its dual-core architecture and high clock speed (240MHz), ensuring flicker-free display updates. Power is managed via an LM2596 Buck Converter, stepping down a 9V DC source to a stable 5V logic rail, isolating the high-current LED load from the microcontroller's internal regulator.

### 2.2 Component Bill of Materials (BOM)
| Component | Specification | Function |
|-----------|---------------|----------|
| **Microcontroller** | ESP32 DevKit V1 (30-Pin) | Central processing and logic control |
| **Display** | 8x8 WS2812B Matrix | Addressable RGB display grid |
| **Audio** | TDK PS110 Passive Buzzer | PWM-driven sound synthesis |
| **Power Regulation** | LM2596 Buck Converter | 9V to 5V DC-DC Step-down |
| **Input Interface** | 5x Tactile Switches | Active-Low user inputs |
| **Power Source** | 9V Battery | Primary power supply |

### 2.3 Pin Configuration & Planning
Careful pin planning was required to avoid ESP32 "Strapping Pins" (pins that determine boot modes).

| Function | GPIO Pin | Configuration | Notes |
|----------|----------|---------------|-------|
| **Button: Left** | GPIO 15 | INPUT_PULLUP | *Relocated from GPIO 2 (Strapping Pin conflict)* |
| **Button: Right** | GPIO 3 | INPUT_PULLUP | *Shared with UART0 RX; Disconnect during upload* |
| **Button: Rotate**| GPIO 4 | INPUT_PULLUP | - |
| **Button: Drop** | GPIO 14 | INPUT_PULLUP | - |
| **Button: Pause** | GPIO 18 | INPUT_PULLUP | - |
| **Audio Out** | GPIO 23 | OUTPUT | PWM signal generation |
| **Display Data** | GPIO 25 | OUTPUT | WS2812B Data Line |

## 3. Circuit Diagram
*(Insert your hand-drawn schematic image here: `![Schematic](assets/images/circuit_diagram.jpg)`)*

The circuit utilizes a "Split Power" topology. The Buck Converter output feeds the LED Matrix and ESP32 `VIN` pin in parallel. This prevents the LED current (up to 2A at full white) from passing through the ESP32 traces, mitigating thermal risks.

## 4. Engineering Challenges & Resolutions

### 4.1 The "Strapping Pin" Conflict (GPIO 2)
**Problem:** Initially, the 'Left' button was mapped to GPIO 2. The input failed to register, and the system became unstable.
**Analysis:** GPIO 2 is an ESP32 strapping pin connected to the on-board LED. During boot, it must be floating or low. The internal circuitry interfered with the weak internal pull-up resistor required for the button.
**Solution:** The button was remapped to GPIO 15, a standard GPIO pin, resolving the conflict without external hardware changes.

### 4.2 UART Interference on GPIO 3
**Problem:** The 'Right' button is mapped to GPIO 3 (RX0). While the button worked during gameplay, uploading new firmware failed with a "Time Out" error.
**Analysis:** GPIO 3 is the Receive (RX) line for the UART interface used to flash the chip. Connecting a button (and ground path) effectively shorted the data stream from the computer.
**Solution:** Implemented a workflow protocol: The GPIO 3 jumper wire is physically disconnected during the bootloader sequence and reconnected immediately after successful flashing.

### 4.3 Input "Ghosting" (Floating Pins)
**Problem:** During early prototyping, the buzzer would trigger random sounds, and pieces would move without user input.
**Analysis:** Long jumper wires acted as antennas, coupling capacitive noise into high-impedance input pins.
**Solution:**
1. **Software:** Enabled internal `INPUT_PULLUP` resistors on all input pins.
2. **Hardware:** Established a solid "Star Ground" topology, ensuring all button ground returns met at a single low-impedance point at the Buck Converter output.

### 4.4 Matrix Orientation
**Problem:** The physical mounting of the LED matrix resulted in the coordinate system being inverted (Tetrominos fell upwards).
**Solution:** Implemented a coordinate transformation layer in the display driver:
`newX = (WIDTH - 1) - x;`
`newY = (HEIGHT - 1) - y;`
This allows the software logic to remain standard while visually correcting the output 180 degrees.

## 5. Software Implementation
The firmware is written in C++ utilizing the Arduino framework. Key modules include:
* **Game Loop:** Non-blocking `millis()` timer implementation to handle gravity and inputs independently.
* **Display Driver:** Uses the `FastLED` library for DMA-based control of the WS2812B LEDs.
* **Audio Driver:** Custom wrapper for the `ledc` peripheral to generate square wave tones for game events.

## 6. Future Improvements
* **Hardware Consolidation:** Migrate the circuit from the solderless breadboard to a permanent perfboard (prototyping board) assembly.
* **Circuit Hardening:** Implement soldered point-to-point wiring to improve mechanical stability and vibration resistance.
* **Enclosure:** Finalize 3D-printed clamshell case design in Fusion 360 to house the consolidated electronics.
* **Power Management:** Integrate a TP4056 Li-Ion charging module to replace the disposable 9V battery.

## 7. License
This project is open-source software licensed under the MIT License.
