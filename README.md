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

**KIVA** is a compact, handheld device that blends the concept of an AI assistant with a powerful suite of security testing tools. The name "KIVA" is derived from:

-   **"Ki"**: Inspired by Lord Krishna (symbolizing wisdom and guidance) and the Japanese concept of "Ki" (energy, spirit).
-   **"Va"**: From "Varta" (Sanskrit for existence, movement, life's flow).

KIVA aims to be a digital "charioteer"—a guide and companion, providing assistance and insights while also offering a robust platform for network analysis and security education.

## ⚠️ Disclaimer

KIVA is intended for **educational and security testing purposes only**. All features should be used in compliance with local laws and regulations. The developers are not responsible for any misuse of this tool. Always obtain proper authorization before testing any network or device you do not own.

## Core Features

KIVA is more than just a concept; it's a feature-rich platform built on a modular and extensible OS.

-   **Modular & Customizable OS:** A lightweight, custom-built operating system with an intuitive, animated UI that is easy to extend.
-   **Dual-Display Interface:** A main 128x64 display for the primary UI and a secondary 128x32 display for status information or auxiliary content like the on-screen keyboard.
-   **Versatile Hardware Control:** Onboard **haptic feedback motor** and **laser pointer** for physical interaction and signaling.
-   **Comprehensive Settings:** Persistent settings for brightness (unified and independent), volume, keyboard layouts, OTA passwords, and attack parameters, all managed via an on-screen interface.
-   **Over-the-Air (OTA) Updates:** Update the firmware seamlessly via a web browser or directly from the SD card.
-   **Utilities:** A built-in **MP3 music player** and a **USB Mass Storage mode** to easily manage files on the SD card from a computer.

## Security & Reconnaissance Suite

KIVA is equipped with a versatile toolkit for network and protocol analysis.

### WiFi Attacks & Analysis

-   **Beacon Spam:** Flood the area with thousands of fake WiFi networks, using either randomly generated SSIDs or custom lists from the SD card.
-   **Deauthentication:** Disrupt network connectivity by sending deauth frames to clients. Can target a specific network or cycle through all nearby APs using broadcast or rogue AP methods.
-   **Evil Twin:** Launch a sophisticated captive portal attack. The device deauthenticates a target network's clients, creates a clone of the AP, and serves a fake login portal to capture credentials.
-   **Probe Request Sniffing:** Passively listen for and log probe requests to discover what WiFi networks nearby devices are searching for.
-   **Handshake Capture:** Sniff the air for WPA2 handshakes (both full 4-way EAPOL and clientless PMKID) and save them to the SD card in `.pcap` format for offline analysis.

### Bluetooth Low Energy (BLE) Attacks

-   **BLE Spam & Spoofing:** Announce a flood of BLE advertisement packets that mimic common devices, causing disruptive pop-ups on a wide range of phones and computers. Includes specific payloads for:
    -   **Sour Apple:** A collection of payloads targeting iPhones, iPads, and Apple Watches.
    -   **Android Fast Pair:** Mimics various Android devices seeking to pair.
    -   **Windows Swift Pair:** Triggers connection pop-ups on modern Windows machines.
    -   **Samsung Devices:** Spoofs nearby Samsung watches and peripherals.

### Host-Based (HID) Attacks

-   **BadUSB (Ducky Script):** Emulates a keyboard when plugged into a computer via USB and automatically executes scripts written in Ducky Script. A powerful tool for automating keyboard-based attacks and tasks.
-   **Bad Bluetooth:** The wireless version of BadUSB. KIVA emulates a BLE keyboard, allowing it to execute Ducky Scripts remotely once a host pairs with it.

### 2.4GHz RF Jamming

-   Leveraging **dual nRF24L01+ modules**, KIVA can perform targeted jamming attacks across the 2.4GHz spectrum.
-   **Supported Protocols:** BLE advertising channels, Bluetooth Classic, specific WiFi channels, and Zigbee.
-   **Techniques:** Utilizes both **noise injection** (sending garbage packets) and **constant carrier** (broadcasting an unmodulated signal) for effective disruption.

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

## Future Implementations Roadmap

The KIVA project is under active development. Key features planned for the near future include:

-   **Headless Mode ("Arm for Boot"):** Pre-configure an attack or action to execute immediately on boot, turning KIVA into a deployable hardware payload.
-   **Advanced Power Management:** Implement a screen timeout and sleep mode to dramatically increase battery life.
-   **Profiles Manager:** A system to save and load entire device configurations (settings, attack parameters) for different operational scenarios.
-   **BLE Remote Control:** A dedicated mode to map the physical buttons to custom keyboard presses or shortcuts, turning the device into a powerful BLE remote for presentations or macros.

---

### Credits

-   [Evil Portals](https://github.com/CodyTolene/Red-Portals) for captive portal inspiration.