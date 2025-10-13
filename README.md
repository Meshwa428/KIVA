
# KIVA: Handheld AI Assistant & Security Tool

<p align="center">
    <img src="data/banner.png" />
</p>

<p align="center">
    <img src="https://img.shields.io/github/last-commit/Meshwa428/KIVA?style=flat-square" />
    <img src="https://img.shields.io/github/repo-size/Meshwa428/KIVA?style=flat-square" />
    <img src="https://img.shields.io/github/stars/Meshwa428/KIVA?style=flat-square" />
    <img src="https://img.shields.io/github/forks/Meshwa428/KIVA?style=flat-square" />
    <img src="https://img.shields.io/github/issues/Meshwa428/KIVA?style=flat-square" />
    <img src="https://img.shields.io/github/issues-pr/Meshwa428/KIVA?style=flat-square" />
    <img src="https://img.shields.io/github/license/Meshwa428/KIVA?style=flat-square" />
</p>

## Introduction

**KIVA** is a compact, handheld device that blends the concept of an AI assistant with a powerful suite of security testing tools. It aims to be a digital "charioteer"—a guide and companion, providing assistance and insights while also offering a robust platform for network analysis and security education.

## ⚠️ Disclaimer

KIVA is intended for **educational and security testing purposes only**. All features should be used in compliance with local laws and regulations. The developers are not responsible for any misuse of this tool. Always obtain proper authorization before testing any network or device you do not own.

## Documentation

For detailed information about the project, please refer to the [**KIVA OS Documentation**](./docs/README.md).

- [**Getting Started**](./docs/getting_started.md): Instructions on how to set up your development environment, build the firmware, and flash it to your device.
- [**Architecture**](./docs/architecture.md): A deep dive into the event-driven architecture that powers KIVA OS.
- [**Features**](./docs/features.md): A detailed look at the security and utility features implemented in KIVA.
- [**Roadmap**](./docs/roadmap.md): An overview of the planned features and future direction of the project.

## Hardware Details

KIVA is built around the **Seeed Studio XIAO ESP32S3 Sense**, chosen for its small form factor, integrated features, and processing power.

| Component              | Details                                                          |
| ---------------------- | ---------------------------------------------------------------- |
| **Microcontroller**    | Seeed Studio XIAO ESP32S3 Sense (Wi-Fi, BT, Camera, Mic)          |
| **Primary Display**    | 128x64 SH1106 OLED Display                                       |
| **Secondary Display**  | 128x32 SSD1306 OLED Display                                      |
| **RF Modules**         | 2x nRF24L01+ Modules (for jamming)                               |
| **Audio Output**       | Small 8Ω 1W speaker with PAM8403 amplifier                       |
| **Storage**            | MicroSD card reader with 32 GB card                              |
| **Power**              | Rechargeable Li-Po battery with TP4056-based charging module     |
| **Input**              | Custom scroll wheel (rotary encoder) & up to 3 physical buttons  |
| **Physical I/O**       | Haptic feedback motor & Laser diode                              |
| **I/O Management**     | PCF8574T I2C GPIO expander & TCA9548A I2C Multiplexer            |

### Credits

-   [Evil Portals](https://github.com/CodyTolene/Red-Portals) for captive portal inspiration.
