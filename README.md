# rp2350A_fpga_board
rp2350A_fpga_board

RP2350 & iCE40 FPGA Development Board
This repository contains the hardware design files for a compact, high-performance development board combining the Raspberry Pi RP2350 MCU and the Lattice iCE40UP5K FPGA. This dual-chip architecture bridges the gap between flexible software execution and high-speed, parallel hardware acceleration, making it ideal for edge-computing, custom digital logic, and advanced interfacing projects.

Technical Specifications
Core Components
Microcontroller: Raspberry Pi RP2350A (ARM Cortex-M33 / Hazard3 RISC-V)

FPGA: Lattice iCE40 UltraPlus (iCE40UP5K-SG48ITR) – 5280 LUTs, 120 Kbit EBR, 1024 Kbit Single Port RAM

Storage: Winbond W25Q128JVS (128 Mbit / 16 MB QSPI Flash) shared/accessible for configuration

Power & Connectivity
Interface: USB Type-C for power, programming, and debugging

Power Management: Dual AP2112K LDO regulators providing stable 3.3V and 1.2V rails; onboard filtering networks for analog and PLL domains

User Interfacing: * Dedicated BOOT and RUN (Reset) buttons for the MCU

User-programmable button (Push-IO4) and status LEDs (Done, RGB)

High-density dual-row breadboard-compatible pin headers exposing GPIOs from both MCU and FPGA
