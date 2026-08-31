# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository layout

Six independent programs, one per physical board plus the PC HMI. There is no top-level build.

| Dir | Target | Role |
|---|---|---|
| `adc_comm/` | STM32F405RGT6, FreeRTOS | Signal sampling (PMT gain/reference DACs) **and Modbus gateway** between the PC and the other four boards |
| `motor_ctrl/` | STM32F407VET6, FreeRTOS | Cover/seal/churn motors, X/Y/Z steppers, lasers, LED, temperature + fans |
| `mfc_ctrl/` | STM32F405RGT6, FreeRTOS | Microfluidic pressure control (5 channels), solenoid + proportional valves |
| `PZT/` | STM32F405RGT6, **bare-metal** | PZT auto-press: 2 step motors, DC-DC drive voltage, current/quantity checks. Uses FreeMODBUS (`Core/Src/modbus/`), not the hand-written stack |
| `led_ctrl/` | STM32F030F4P6, **bare-metal** | CS_LED_Ctrl V1.0: 14 RGB LEDs via a bit-banged TLC5957, door-lock solenoid + feedback. **The only project with two build configurations** — see Build |
| `SeekCytometer_Peripheral/` | Qt 6 Widgets (CMake) | PC HMI, Modbus-RTU master over serial |

`reference/` holds datasheets (DAC8568, AD5724R, TLC5957) and board schematics.
Design and planning documents for `led_ctrl` live outside this repo, under `../doc/design/`.

## Build

### Firmware (STM32CubeIDE managed-make projects)

`Debug/`/`Release/` are gitignored, so the generated makefiles only exist after CubeIDE has built the project once. With them present, build from the CLI:

```bash
export PATH="/opt/st/stm32cubeide_2.0.0/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.linux64_1.0.100.202509120712/tools/bin:$PATH"
make -C adc_comm/Debug all -j8      # same for motor_ctrl, mfc_ctrl, PZT
```

`led_ctrl` is the exception: it has no `Debug/`. The STM32F030F4P6 has one USART,
so the debug shell and the Modbus slave cannot coexist and are selected by
`-DUSART1_ROLE=` in two configurations, both `-Os`:

```bash
make -C led_ctrl/Debug_Shell all -j8    # USART1_ROLE=1, bench bring-up
make -C led_ctrl/Debug_Modbus all -j8   # USART1_ROLE=2, what ships
```

16 KB of Flash is the binding constraint on `Debug_Shell`; run
`arm-none-eabi-size` after any change to it. No floating point, no `malloc`, no
`printf` — `debug_shell.c` carries its own minimal formatter, because reaching
newlib's stdio pulls in about 4.5 KB.

From a clean clone (no `Debug/`), generate them headlessly:

```bash
/opt/st/stm32cubeide_2.0.0/headless-build.sh -data /tmp/ws -import adc_comm -build adc_comm/Debug
```

`.project`/`.cproject` are deliberately tracked (see `.gitignore`) — they carry the toolchain, includes, defines and linker script, so don't "clean them up".

### HMI

```bash
cd SeekCytometer_Peripheral
cmake -S . -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.9.0/gcc_64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
```

The `find_package(QT NAMES Qt6 Qt5 ...)` will silently pick the system Qt5 if `CMAKE_PREFIX_PATH` is omitted; the project is developed against Qt 6.9. New source files must be added explicitly to the `qt_add_executable(...)` list in `CMakeLists.txt` (there is no glob).

There is no test suite in this repo. Firmware is exercised through the per-board debug shell (below); the HMI through the running UI.

## Architecture

### Everything is one flat Modbus holding-register map

The PC HMI talks to **only one slave address, `0x11` (adc_comm)**, over Modbus-RTU 115200 8N1. `adc_comm` routes each request to the right board based purely on the register address:

| Range | Owner | On-wire slave addr |
|---|---|---|
| 40001–40100 | adc_comm (local) | 0x11 |
| 40101–40200 | motor_ctrl | 0x22 |
| 40201–40300 | mfc_ctrl | 0x33 |
| 40301–40400 | PZT | 0x44 |
| 40401–40500 | led_ctrl | 0x55 |

A request must not straddle a block boundary — `adc_comm/Core/Src/modbus_slave.c` returns `TARGET_INVALID` (illegal-data-address exception) if it does.

**The register map is duplicated in five places and there is no generator.** Adding or moving a register means editing all of:
- `SeekCytometer_Peripheral/ModbusRegistersTable.h` — the HMI's view of the whole 400-register map, plus every command/status enum value
- the owning board's `Core/Src/modbus_local_table.c` — `MB_Local_RegInit()` binds each *block-relative index* to a `uint16_t` variable via `MB_Slave_DefineReg(idx, &var)`; unbound indices read back as an exception
- the owning board's `Core/Inc/modbus_defs.h` (`MB_LOCAL_REG_START`) if block bases change; `adc_comm/Core/Inc/modbus_gateway.h` owns the block layout
- PZT instead maps registers in `PZT/Core/Src/interface.c` (`MODBUS_ChannelRegs`, guarded by `USE_MODBUS`) behind FreeMODBUS's `eMBRegHoldingCB` in `Core/Src/modbus/port.c`

Keep the HMI's `#define` names and the firmware's `MB_Slave_DefineReg` indices in sync, or writes land on the wrong variable with no error.

### Control-word command pattern

Boards are not commanded by writing a value and having it take effect. Each block has a **control word at index 0** (bitfield, one bit per subsystem: cover/seal/churn/temp/stepper/laser/LED, see `CtrlWord_t` in `motor_ctrl/Core/Src/modbus_local_table.c`) plus per-subsystem `*_CMD` / `*_DATA` registers. The HMI writes CMD + DATA first, then toggles the control-word bit; `MB_CommandParse()` fires on the **0→1 edge** (`ctrlWord != lastCtrlWord`) and dispatches to the owning module. Status flows back through separate read-only registers (roughly index 47+), updated by `MB_UpdateStatus()`.

### Timing constraints that are easy to break

- Gateway transaction timeout is 200 ms (`MB_DEFAULT_TIMEOUT_MS`), deliberately below the HMI's 1000 ms timeout and 400 ms poll period so a dead sub-board yields an exception rather than a pile-up. Changing any one of the three means checking the other two.
- Sub-board slave responses must be sent by DMA/IT as one contiguous frame. Blocking or splitting a response breaks the master's IDLE-line framing.
- The HMI polls in a 4-step round-robin driven by `MainWindow::m_count` (`onNeedUpdateStatus`), and `handleResponse` demultiplexes replies **by `m_count` and register count** — change a poll's `quantity` and you must change the matching `regs.size()` check.

### UART assignments

Each board has a `Core/Inc/bsp_uart.h` mapping roles to HAL handles — read it before touching UART code.

- `adc_comm`: huart3 shell (PB10/PB11), huart6 ↔ PC, huart5 ↔ motor, huart1 ↔ MFC, huart4 ↔ PZT, **huart2 ↔ led_ctrl** (huart2/PA2/PA3 used to be the shell; it was moved so the LED board could have a link)
- `motor_ctrl`: huart1 shell, huart3 ↔ adc_comm (slave), huart4 → stepper drivers (**it is a Modbus master here**, CiA402-style register map in `Core/Inc/stepper_motor_protocol.h`)
- `mfc_ctrl`: huart2 shell, huart1 ↔ adc_comm
- `led_ctrl`: huart1 is **either** the shell **or** the Modbus slave, never both
- `PZT`: FreeMODBUS at 115200, shell on huart2

### FreeRTOS task startup

The three RTOS boards create everything from `USER CODE BEGIN RTOS_THREADS` in `Core/Src/freertos.c` (`Shell_StartDebugTask()`, `MB_Slave_StartTask()`, then per-subsystem `*_StartTask()`). Add new tasks there, not in `main.c`.

### Debug shell

`debug_shell.c` is shared (copied) across the three RTOS boards: a UART ring-buffer shell with `LOG_INFO`/`LOG_ERROR` macros and command registration via `Shell_RegisterCommand(s)`. Board-specific commands live in `shell_commands_register.c` (`sample_control`/`debug_commands.c` on adc_comm) and follow a `cmd -x [args]` single-letter-flag convention.

### HMI internals

`ModbusMaster` is a singleton wrapping `QSerialPort` with a request queue, offering both blocking and async APIs; `MainWindow` owns one dock widget per subsystem and wires them to it. Persistent configuration (chip/lens positions, laser config, hydraulic params) goes to **PostgreSQL** via `QPSQL` — `database/DatabaseManager.cpp` plus one DAO per table; schema in `sql/create_tables.sql`.

## Working with CubeMX-generated code

All four firmware projects are `.ioc`-driven. Hand-written code inside generated files must stay inside `/* USER CODE BEGIN X */ ... /* USER CODE END X */` markers — a CubeMX regeneration wipes anything outside them. Notably `motor_ctrl`'s TIM7 software-PWM (churn motor) config is hand-written in `Core/Src/tim.c` `USER CODE 1`; keep additions of that kind in the same section rather than spreading them across the file.
