
# KIVA Features

KIVA is equipped with a versatile toolkit for network analysis, security testing, and everyday utility.

## Security & Reconnaissance Suite

### WiFi Attacks & Analysis

-   **Beacon Spam:** Flood the area with thousands of fake WiFi networks, using either randomly generated SSIDs or custom lists from the SD card.
-   **Deauthentication:** Disrupt network connectivity by sending deauth frames to clients. Can target a specific network or cycle through all nearby APs.
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

-   **BadUSB (Ducky Script):** Emulates a keyboard when plugged into a computer via USB and automatically executes scripts written in Ducky Script.
-   **Bad Bluetooth:** The wireless version of BadUSB. KIVA emulates a BLE keyboard, allowing it to execute Ducky Scripts remotely once a host pairs with it.

### 2.4GHz RF Jamming

-   Leveraging **dual nRF24L01+ modules**, KIVA can perform targeted jamming attacks across the 2.4GHz spectrum.
-   **Supported Protocols:** BLE advertising channels, Bluetooth Classic, specific WiFi channels, and Zigbee.

## Core Features & Utilities

-   **Modular & Customizable OS:** A lightweight, custom-built operating system with an intuitive, animated UI that is easy to extend.
-   **Dual-Display Interface:** A main 128x64 display for the primary UI and a secondary 128x32 display for status information.
-   **Versatile Hardware Control:** Onboard **haptic feedback motor** and **laser pointer**.
-   **Comprehensive Settings:** Persistent settings for brightness, volume, keyboard layouts, OTA passwords, and attack parameters.
-   **Over-the-Air (OTA) Updates:** Update the firmware seamlessly via a web browser or directly from the SD card.
-   **Utilities:** A built-in **MP3 music player** and a **USB Mass Storage mode** to easily manage files on the SD card from a computer.
-   **Snake Game:** A classic snake game, just for fun.
