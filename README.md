# 🏭 FreeRTOS Based Real-Time Equipment Health Monitoring and Predictive Alert System using LPC2129

**A FreeRTOS-based embedded monitoring node with predictive alerting on the Linux side**

An ARM7 (LPC2129) board runs FreeRTOS to sense a machine's temperature, load, and
ambient light in real time, then hands the data off to a Linux server that logs it,
predicts threshold breaches before they happen, and raises email + live-dashboard
alerts the moment a real fault occurs.

**MCU:** LPC2129 (ARM7TDMI) · **RTOS:** FreeRTOS V10.4.3 LTS · **Toolchain:** Keil uVision · **Server:** C + OpenSSL

---

## 📋 Table of Contents

- [What This Project Does](#-what-this-project-does)
- [System Architecture](#-system-architecture)
- [Hardware Used](#-hardware-used)
- [Features](#-features)
- [Tech Stack](#-tech-stack)
- [Repository Layout](#-repository-layout)
- [FreeRTOS Task Design](#-freertos-task-design)
- [Getting Started](#-getting-started)
- [Configuration Reference](#-configuration-reference)
- [Known Limitations & Honest Caveats](#-known-limitations--honest-caveats)
- [Author](#-author)

---

## 🔎 What This Project Does

Industrial machines fail expensively and often without warning. This project is a
low-cost, two-tier monitoring node that watches a machine's **temperature**, **load**,
and **ambient light**, and does two things most simple monitors don't:

1. **Predicts problems before they happen** — not just "temperature is 85°C," but
   "temperature is rising and will cross 80°C in the next 4 seconds," so someone can
   act before the machine actually faults.
2. **Keeps itself alive** — a hardware watchdog and per-task heartbeat monitor mean
   that if any FreeRTOS task silently hangs, the microcontroller resets itself instead
   of failing invisibly.

The embedded side (**LPC2129 + FreeRTOS**) does the real-time sensing, local display,
and adaptive alarm. It streams every reading over UART to a **Linux server**, which
logs it, runs the predictive threshold model, emails an alert on a real fault, and
serves a live status feed to a terminal dashboard client.

---

## 🏗 System Architecture

```mermaid
flowchart LR
    subgraph MCU["LPC2129 · FreeRTOS"]
        A[Onboard ADC<br/>Temperature] --> S[sensorTask]
        B[MCP3204 SPI ADC<br/>Load + Light] --> S
        C[DS1307 I2C RTC<br/>Date/Time] --> S
        S -->|senpro queue| P[processingTask]
        P -->|LCD| L[16x2 HD44780 LCD]
        P -->|proalr queue| AL[alertTask]
        P -->|prour queue| U[uartTask]
        AL --> BZ[Buzzer]
        AL --> LED[LEDs]
        W[watchdog task] -.heartbeat check.-> S
        W -.heartbeat check.-> P
        W -.heartbeat check.-> U
        W --> HWDOG[(Hardware Watchdog)]
    end

    U -->|UART 9600 baud| SER[/dev/ttyUSB1]
    SER --> SRV[Linux Server]
    SRV --> CSV[(data_log.csv)]
    SRV --> PRED{Predictive<br/>Threshold Model}
    SRV -->|SMTP + STARTTLS| MAIL([📧 Email Alert])
    SRV <-->|TCP Socket| CLI[Linux Client<br/>Color Terminal UI]
```

---

## 🔩 Hardware Used

| Component | Part / Interface | Role |
|---|---|---|
| **Microcontroller** | **LPC2129** (ARM7TDMI-S, Philips/NXP) | Runs FreeRTOS, all sensor acquisition and local control logic |
| **RTC (Real-Time Clock)** | **DS1307**, I2C (address `0xD0`/`0xD1`) | Timestamps every sensor reading (date + time) |
| **External ADC** | **MCP3204**, 4-channel 12-bit SPI ADC | Reads the load potentiometer and LDR (light) sensor |
| **Temperature sensor** | Analog sensor on the LPC2129's onboard ADC (channel 1) | *Inferred from the scaling formula in `adc_driver.c`/`main.c` (10 mV/°C with a 500 mV offset — consistent with a TMP36-family sensor); confirm the exact part if this goes into a formal BOM* |
| **Display** | 16×2 character LCD, HD44780-compatible, 4-bit interface | Local readout of date/time, temperature, load, and light |
| **Alert actuators** | Buzzer **and** LEDs (GPIO-driven) | Blink/beep rate scales with how far a reading exceeds threshold |
| **Load input** | Potentiometer, wired to MCP3204 channel | Simulates/represents mechanical load reading |
| **Light input** | LDR (light-dependent resistor), wired to MCP3204 channel | Ambient light sensing |
| **Serial bridge** | USB-to-serial adapter (`/dev/ttyUSB1` on the Linux side) | Carries UART telemetry from the MCU to the Linux server |

---

## ✨ Features

- **Multi-sensor acquisition** — onboard ADC for temperature, MCP3204 SPI ADC for load
  and light, DS1307 I2C RTC for date/time — all polled once per second by `sensorTask`.
- **Fully preemptive RTOS design** — 5 FreeRTOS tasks, 3 message queues, and 1 binary
  semaphore coordinate sensing, processing, display, alerting, telemetry, and health
  monitoring without any single task blocking another.
- **Self-healing via watchdog** — `watchdog` task tracks a heartbeat counter for
  `sensorTask`, `processingTask`, and `uartTask`. If any of them misses 3 consecutive
  checks, the hardware watchdog stops being fed and the MCU resets itself — no manual
  intervention needed.
- **Local live display** — date, time, temperature, load %, and light % shown in
  real time on the 16×2 LCD.
- **Adaptive local alarm** — `alertTask` doesn't just turn the buzzer/LED on or off;
  the blink/beep rate speeds up as the reading climbs further past threshold (500ms
  at 80+, down to 50ms at 95+), giving an audible sense of *how* urgent it is.
  
- **Predictive alerting (Linux side)** — the server doesn't wait for a threshold
  breach. It tracks the rate of change over the last 5 readings and warns
  ("Temperature Raising, In 3.2 seconds Reach 80") *before* the value actually
  crosses 80, giving a real head start.
- **Persistent CSV logging** — every reading is appended to `data_log.csv` with a
  header row, ready to open in Excel/Sheets or feed into further analysis.
- **Email alerting** — on an actual (not predicted) fault, the server sends an
  SMTP+STARTTLS email with the exact date, time, and which reading(s) triggered it.
- **Live color-coded dashboard** — the Linux client connects over TCP and prints
  green (normal), yellow (predictive warning), or red (active alert) status blocks
  to the terminal in real time.
- **Resilient SMTP connection** — if the mail send fails mid-operation, the server
  automatically reconnects and retries rather than silently dropping the alert.

---

## 🧰 Tech Stack

**Embedded**
- LPC2129 (ARM7TDMI-S), Keil uVision / ARM (RVDS-style) toolchain
- FreeRTOS V10.4.3 LTS (Keil ARM7/LPC2000 port)
- Peripherals: onboard ADC, SPI (MCP3204), I2C (DS1307), UART0, GPIO-bitbanged LCD

**Linux**
- C, POSIX sockets, termios (serial I/O)
- OpenSSL (SMTP STARTTLS email delivery)

---

## 📁 Repository Layout

```
Embedded/
├── Application/                 # Application-layer source
│   ├── main.c                    # Tasks, queues, semaphore, watchdog wiring
│   ├── header.h                   # Shared prototypes/types for all drivers
│   ├── adc_driver.c                # Onboard ADC — temperature channel
│   ├── i2c_driver.c                 # I2C driver — DS1307 RTC read/write
│   ├── spi_driver.c                  # SPI driver — MCP3204 load + light channels
│   ├── uart_driver.c                  # UART0 driver + int/float print helpers
│   └── lcd_driver.c                    # 4-bit HD44780-style LCD driver
│
├── Startup/
│   └── Startup.s                # Keil/Philips LPC2000 reset + vector table
│
└── FreeRTOS/                    # Vendored kernel — V10.4.3 LTS Patch 3
    ├── Config/
    │   └── FreeRTOSConfig.h      # Tick rate, heap size, priorities
    └── Source/
        ├── tasks.c, list.c, queue.c, heap_2.c   # Actively used by this project
        ├── timers.c, croutine.c, event_groups.c # Vendored, NOT used — see caveats
        ├── Include/                              # All kernel headers
        └── Portable/Keil/ARM7_LPC2000/            # ARM7 port layer
            ├── port.c            # Stack init, critical sections, tick timer
            ├── portASM.s          # Context switch / SVC yield / tick ISR (ARM asm)
            ├── portmacro.h         # Port-specific types & macros
            └── portmacro.inc        # SAVE/RESTORE_CONTEXT macros for portASM.s

Linux/
├── src/
│   ├── server.c                 # Serial reader, predictive alerts, SMTP, TCP server
│   └── client.c                  # TCP client, color-coded terminal dashboard
└── include/
    └── header_linux.h            # Shared socket/SSL/termios includes
```

---

## ⚙️ FreeRTOS Task Design

| Task | Priority | Responsibility |
|---|---|---|
| `sensorTask` | 3 | Reads onboard ADC / MCP3204 SPI / DS1307 I2C; pushes sample to `senpro` queue every 1s |
| `processingTask` | 2 | Consumes `senpro`, checks thresholds, drives LCD, forwards to `prour` + `proalr` |
| `alertTask` | 2 | Blocks on `alert` semaphore, drives buzzer **and** LEDs at a threshold-scaled rate |
| `uartTask` | 2 | Consumes `prour`, streams formatted telemetry over UART0 |
| `watchdog` | 1 | Heartbeat-checks all tasks; feeds hardware WDT only while all are alive |

**Sync primitives:** `senpro`, `prour`, `proalr` (queues, depth 10) + `alert` (binary semaphore)

---

## 🚀 Getting Started

### Embedded (Keil uVision)
1. Create a new uVision project targeting **LPC2129**.
2. Add every `.c` file from `Embedded/Application`, `Embedded/FreeRTOS/Source`
   (including `Portable/Keil/ARM7_LPC2000`), and `Embedded/Startup/Startup.s`.
3. Add to the include path:
   `Embedded/Application`, `Embedded/FreeRTOS/Config`,
   `Embedded/FreeRTOS/Source/Include`, `Embedded/FreeRTOS/Source/Portable/Keil/ARM7_LPC2000`.
4. Wire up: DS1307 on I2C, MCP3204 on SPI (potentiometer + LDR on two channels),
   analog temperature sensor on the onboard ADC channel 1, 16×2 LCD on the GPIO
   pins used in `lcd_driver.c`, buzzer + LEDs on the GPIO pins used in `main.c`.
5. Build and flash. Toolchain is **Keil ARM (RVDS-style)** — `port.c`/`portASM.s`
   use `__irq`, `__disable_irq()`, and Keil ARM assembler syntax directly.

### Linux (server + client)
```bash
# Build
gcc Linux/src/server.c -I Linux/include -o server -lssl -lcrypto
gcc Linux/src/client.c -I Linux/include -o client

# Run
./server <port> <bind-ip>     # e.g. ./server 5000 0.0.0.0
./client <port>                # connects to 127.0.0.1:<port>
```
`server.c` requires OpenSSL (`libssl`, `libcrypto`) and a serial adapter at `/dev/ttyUSB1`.

---

## 🔧 Configuration Reference

| Setting | Location | Value |
|---|---|---|
| CPU clock | `FreeRTOSConfig.h` | 60 MHz |
| Tick rate | `FreeRTOSConfig.h` | 1000 Hz |
| Heap size | `FreeRTOSConfig.h` | 13 KB (`heap_2`) |
| Max priorities | `FreeRTOSConfig.h` | 4 |
| UART baud | `uart_driver.c` | 9600 |
| Alert threshold | `main.c` | Temp / Load ≥ 80 |
| Watchdog timeout | `main.c` | 5s HW timeout, 3-miss detection window |

---

## ⚠️ Known Limitations & Honest Caveats

- **Credentials:** `server.c` currently has SMTP (Gmail) credentials hardcoded for
  the mail-alert feature. Rotate the app password and move this to a config file
  or environment variable before making the repository public.
- **Vendored kernel modules:** `timers.c/h`, `croutine.c/h`, and `event_groups.c/h`
  ship with the FreeRTOS source tree for completeness but aren't exercised by this
  application — only tasks, queues, the binary semaphore, lists, and `heap_2` run.

---

## 👤 Author

**Chalampalem Uday Kiran**
Embedded Systems Engineer — Bare-metal ARM7, FreeRTOS, CAN, automotive & industrial systems
📧 udayer45@gmail.com · 💻 [github.com/udayer45-cloud](https://github.com/udayer45-cloud) · 🔗 [linkedin.com/in/udaykiran1807](https://linkedin.com/in/udaykiran1807)
