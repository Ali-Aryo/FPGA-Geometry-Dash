# FPGA Geometry Dash

## Overview
This project is a bare-metal hardware implementation of the popular game "Geometry Dash", designed to run directly on an FPGA SoC. Written entirely in C/C++, it bypasses traditional operating systems to interact directly with the FPGA's underlying hardware IP blocks. The game features real-time physics, custom VGA rendering, hardware-accelerated random level generation, and dual-core Asymmetric Multiprocessing (AMP) for perfectly synchronized high-fidelity audio playback.

## Hardware Platform
*   **Target Architecture:** Xilinx Zynq-7000 SoC (Dual-Core ARM Cortex-A9).
*   **Core 0 (Game Engine & Video):** Responsible for the game's state machine, physics (gravity, jumps, collision detection), 60 FPS frame timing, UART input parsing, GPIO interrupt handling, and managing the VGA frame buffer.
*   **Core 1 (Audio Engine):** Dedicated entirely to audio processing. It handles I2C configuration for the onboard audio codec and continuously streams `.WAV` data via I2S, ensuring audio playback does not block or slow down video rendering.
*   **Peripherals & Interfaces:**
    *   **SD Card:** Used for dynamically loading large `.WAV` audio assets directly into DDR memory to swap soundtracks between levels.
    *   **VGA:** Custom IP and driver for direct pixel manipulation and sprite drawing.
    *   **GPIO & Interrupts (GIC):** Hardware switches dictate the game modes, while buttons trigger hardware interrupts for jump commands and sprite selection.
    *   **UART:** Serial interface utilized for text input (setting the player's name).

## Low-Level Architecture & How It Works

### 1. Asymmetric Multiprocessing (AMP)
To maximize performance, the game leverages both ARM Cortex-A9 processors independently. Core 0 initializes the system hardware, loads SD card assets into memory, and manually "wakes up" Core 1 by writing its starting address to the OCM (On-Chip Memory) and issuing an ARM `sev` (Send Event) instruction. From then on, Core 0 sends audio commands (Play, Pause, Reset) to Core 1 via a tightly controlled shared memory pointer at `0xFFFF0000`. Core 1 invalidates its data cache (`Xil_DCacheInvalidateRange`) in its main loop to read these commands accurately without hitting stale cache data.

### 2. State Machine & Frame Limiter
The core logic resides in a strict Finite State Machine (Menu, Gameplay, Pause, Endless, Naming, Audio Levels). A hardware global timer (`XTime_GetTime()`) is used to implement a precise frame limiter that guarantees the main game loop executes exactly at 60 FPS (`COUNTS_PER_SECOND / 60`), ensuring consistent physics and scroll speed across different hardware modes.

### 3. Rendering & Collision Detection
*   **VGA Rendering:** Backgrounds, text, and sprites are painted directly into a predefined DDR memory region (`image_buffer_pointer`). Sprites are updated by erasing the previous footprint and drawing the new one to minimize memory bus overhead.
*   **Physics:** Bounding-box collision algorithms check for intersections between the player sprite and incoming spike coordinates (`game_physics.cc`). 
*   **Hardware Random Number Generation:** The "Endless Mode" dynamically generates spikes using a hardware-based Linear-Feedback Shift Register (LFSR) instantiated in the FPGA fabric. The software reads this register and evaluates it against an `LFSR_SPIKE_THRESHOLD` to determine where to place obstacles.

### 4. Memory Management & Cache Coherency
Since the project does not use an OS to manage virtual memory, raw physical memory addresses are utilized extensively. The SD card loader pushes audio data straight into DDR (e.g., `AUDIO_DATA_ADDR`). Because the two cores share this DDR memory, Core 0 flushes its data cache (`Xil_DCacheFlushRange()`) immediately after loading SD assets to ensure the data is written out to main memory, making it accessible to Core 1's audio DMA/playback routines.