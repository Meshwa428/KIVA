## Big Chunky Summary of KIVA Attack Modules

### Category 1: Bluetooth Low Energy (BLE) Attacks

These attacks target the BLE protocol using the `NimBLE-Arduino` library. They are effective against a huge range of modern devices.

#### 1. BLE Spoofing & "Sour Apple"
*   **Method Names:** `BleSpoofer`, `BleSpam`
*   **Concept:** This attack involves broadcasting specially crafted BLE advertisement packets that mimic common devices (Apple, Android, Windows) or services. Receiving devices (like iPhones or Android phones) see these packets and automatically trigger UI pop-ups for pairing, setup, or other actions, causing a denial-of-service or confusion. The "Sour Apple" attack is the specific name for the collection of Apple-related BLE spam payloads.
*   **Implementation Plan:**
    1.  **Create a Manager Class:** `BleSpoofer.h` and `BleSpoofer.cpp`. This class will manage the state (`isActive`, `currentPayloadType`) and contain the advertisement logic.
    2.  **Define Payloads:** The core of the attack is the data. In `BleSpoofer.cpp`, define static constant byte arrays for each attack type. You can find these well-documented payloads in the Flipper Zero community resources.
        ```cpp
        // Example structure in BleSpoofer.cpp
        // From Flipper Zero Unleashed firmware (flipperzero-firmware-unleashed/lib/ble_spam)
        static const uint8_t apple_tv_payload[] = { 0x1E, 0xFF, 0x4C, 0x00, /* ... more bytes */ };
        static const uint8_t airpods_payload[] = { 0x1E, 0xFF, 0x4C, 0x00, /* ... more bytes */ };
        static const uint8_t fast_pair_payload[] = { 0x11, 0x06, 0x2C, 0xFE, /* ... more bytes */ };
        ```
    3.  **Core Logic:**
        *   The `BleSpoofer::start(AttackType type)` method will initialize NimBLE using `NimBLEDevice::init()`. It will set a loop to cycle through the selected payloads (e.g., just Apple payloads, or all of them for "Chaos Mode").
        *   The `BleSpoofer::loop()` or a dedicated task will perform the spamming. Inside the loop:
            *   Get the advertising object: `NimBLEAdvertising *pAdvertising = NimBLEDevice::getAdvertising();`
            *   Set the manufacturer data for the current payload: `pAdvertising->setManufacturerData(std::string((char*)payload, sizeof(payload)));`
            *   Start advertising for a very short duration: `pAdvertising->start(150, nullptr);` (e.g., 150ms). This short burst is key to fast cycling.
            *   `delay(100);` before moving to the next payload to avoid overwhelming the ESP32.
    4.  **Menu Flow:**
        *   In `App.cpp`, add a "BLE Tools" `CarouselMenu` or `GridMenu`.
        *   This menu should lead to a `SplitSelectionMenu` to choose the spam target: "Apple", "Android/Windows", or "Chaos Mode (All)".
        *   Selecting an option configures and then navigates to a new, simple `BleSpamActiveMenu` which calls `bleSpoofer.start()` on enter and `bleSpoofer.stop()` on exit.

---
### Category 2: Advanced WiFi Credential & Data Attacks

These attacks focus on capturing credentials or data without relying on a captive portal.

#### 2. PMKID Capture
*   **Method Name:** `PmkidCapturer`
*   **Concept:** A faster, clientless attack to get a crackable hash from WPA2 networks. By attempting to associate with a target AP, we can often provoke it into sending us the first EAPOL message of the 4-way handshake. This message contains the PMKID, which can be cracked offline with tools like `hashcat` to reveal the network password.
*   **Implementation Plan:**
    1.  **Create Manager Class:** `PmkidCapturer.h` and `.cpp`.
    2.  **Core Logic:**
        *   The `start(target_network)` method will switch the WiFi hardware to STA mode and enable promiscuous mode with a packet handler callback: `esp_wifi_set_promiscuous(true); esp_wifi_set_promiscuous_rx_cb(&packet_handler);`.
        *   The `packet_handler` is the key. It needs to parse raw 802.11 frames. It will ignore everything *except* EAPOL packets (EtherType `0x888e`) from the target AP's BSSID.
        *   To trigger the response, the ESP32 sends a single "Authentication Request" and "Association Request" frame to the target AP. You will have to craft these frames manually.
        *   When the EAPOL packet is received in the handler, parse it to find the WPA Key Data field and extract the PMKID.
        *   **Crucially, format the output correctly.** The captured data must be saved to the SD card in the `hashcat -m 22000` format: `PMKID*MAC_AP*MAC_STA*SSID_HEX*<notes>`.
    3.  **Menu Flow:**
        *   `WiFi Tools -> Advanced Attacks -> PMKID Capture`.
        *   This leads to the `WifiListMenu`. The `WifiListDataSource` will need to be configured so that `onItemSelected` calls `pmkidCapturer.start(selected_network)` instead of trying to connect.
        *   A new active menu, `PmkidCaptureActiveMenu`, will display the status: "Trying to associate...", "Listening for EAPOL...", "Success! PMKID captured to SD.", or "Failed (AP may not be vulnerable)."

#### 3. Probe Request Sniffing & KARMA Attack
*   **Method Name:** `ProbeSniffer`
*   **Concept:** Passively listen for 802.11 Probe Request frames, which are broadcast by devices searching for previously known WiFi networks. You log the SSIDs from these requests. The KARMA attack is the next step: you create a fake, open AP with one of the sniffed SSIDs, tricking the device into automatically connecting.
*   **Implementation Plan:**
    1.  **Create Manager Class:** `ProbeSniffer.h` and `.cpp`. It will hold a `std::vector<std::string>` of unique SSIDs found.
    2.  **Core Logic:**
        *   The main logic resides in a promiscuous packet handler. The device will channel-hop through 2.4GHz channels (1-14).
        *   The handler will parse 802.11 frames, specifically looking for Management Frames (type `0`) with subtype `Probe Request` (`0x04`).
        *   When a probe request is found, parse its "Tagged Parameters" section to find Tag `0` (SSID). If the SSID is not broadcast (`length > 0`) and not already in your list, add it.
    3.  **Menu Flow:**
        *   `WiFi Tools -> Recon -> Probe Sniffer`.
        *   This opens a new `ListMenu` subclass, `ProbeListMenu`, which dynamically displays the contents of the `probeSniffer`'s SSID vector. The list should refresh every few seconds.
        *   When a user selects an SSID from this list, it should ask "Launch KARMA Attack?".
        *   If "Yes", it reuses your existing **Evil Twin** flow. It calls `evilTwin.prepareAttack()`, sets the portal, and then calls `evilTwin.start()` using the sniffed SSID as the target and an open authentication configuration.

#### 4. Passive Packet Sniffing
*   **Method Name:** `PacketSniffer`
*   **Concept:** A general-purpose tool to capture all raw WiFi traffic on a specific channel and save it to a `.pcap` file on the SD card for later analysis in tools like Wireshark.
*   **Implementation Plan:**
    1.  **Create Manager Class:** `PacketSniffer.h` and `.cpp`.
    2.  **File I/O:** This is the most complex part. You must correctly write the PCAP file format.
        *   `start()`: Open a new file (e.g., `/data/captures/capture_[timestamp].pcap`). Write the 24-byte PCAP Global Header.
        *   `packet_handler`: For each packet received, write a 16-byte PCAP Per-Packet Header (containing timestamp, captured length, original length) followed by the raw packet data itself.
        *   To avoid trashing the SD card, buffer packets in RAM (e.g., in a `std::vector`) and write them to the file in chunks of a few KB.
    3.  **Core Logic:** The `start(channel)` method sets the ESP32 to the specified channel and enables the promiscuous callback. The callback's only job is to pass the raw packet data to the file-writing buffer.
    4.  **Menu Flow:**
        *   `WiFi Tools -> Recon -> Packet Sniffer`.
        *   Reuse your `ChannelSelectionMenu` to pick a single channel to monitor.
        *   This leads to a `SnifferActiveMenu` that shows real-time stats (Packets Captured, File Size) and says "Press BACK to Stop". `onExit` calls `sniffer.stop()`, which finalizes and closes the `.pcap` file.

#### 5. Hidden SSID Decloaking
*   **Concept:** Defeat "hidden" networks by forcing them to reveal their name. This is done by sending a deauth packet to a connected client, then sniffing the Probe Request it sends upon reconnecting, which will contain the SSID.
*   **Implementation Plan:**
    1.  **Enhance Existing Classes:** This is not a new manager, but an enhancement to `WifiManager` and `WifiListDataSource`.
    2.  **Detection:** In `WifiManager`, when a network scan result has a zero-length SSID, flag it as hidden.
    3.  **Decloaking (Active Method):**
        *   In your `WifiListMenu`, if the user selects a hidden network, show a new option: "Decloak SSID".
        *   This triggers a deauth attack against the broadcast address for that BSSID.
        *   Simultaneously, a promiscuous packet sniffer (like the one from the Probe Sniffer) listens for a Probe Request containing that BSSID. Once found, the SSID is extracted.
    4.  **UI Update:** The `WifiListDataSource` should update the label for that network item from `[Hidden Network]` to `RevealedSSID [Hidden]` and force a redraw.

---
### Category 3: WiFi Denial of Service (DoS) Attacks

#### 6. WPA3 Authentication Flood
*   **Method Name:** `Wpa3Flooder`
*   **Concept:** Overwhelm a WPA3-enabled Access Point by exploiting its computationally expensive SAE "Dragonblood" authentication mechanism. By sending a high volume of fake "Authentication Commit" frames, you force the AP's CPU to perform complex cryptographic calculations for each one, leading to resource exhaustion and denying service to legitimate users.
*   **Implementation Plan:**
    1.  **Create Manager Class:** `Wpa3Flooder.h` and `.cpp`.
    2.  **Packet Crafting:** This is a raw packet injection attack. You will need to manually construct an 802.11 Authentication frame. The key is to set the "Authentication Algorithm" field to `3` (for SAE) and include the necessary SAE Commit parameters in the frame body.
    3.  **Core Logic:** The `loop()` function will be a tight loop that does the following:
        *   Generate a random source MAC address.
        *   Craft the SAE Authentication Commit frame using the target AP's BSSID and the random source MAC.
        *   Use `esp_wifi_80211_tx()` to inject the packet.
        *   Repeat as fast as possible.
    4.  **Menu Flow:**
        *   `WiFi Tools -> DoS Attacks -> WPA3 Auth Flood`.
        *   Leads to `WifiListMenu`. Your `onItemSelected` logic will need to check if the network is WPA3 capable (via scan results) and then start the flooder.
        *   A simple "Active" menu shows that the attack is running.

---
### Category 4: HID Emulation Attacks (USB & Bluetooth)

These attacks make the KIVA device pretend to be a Human Interface Device (keyboard/mouse).

#### 7. BadUSB (Ducky Script Executor)
*   **Concept:** When the KIVA is plugged into a computer's USB port, it identifies itself as a keyboard and automatically executes a pre-written script to perform actions like opening a terminal, downloading malware, or exfiltrating data.
*   **Implementation Plan:**
    1.  **Library:** Add `adafruit/Adafruit TinyUSB Library` to your `platformio.ini`.
    2.  **Create Manager Class:** `HidManager.h` and `.cpp`.
    3.  **Ducky Script Parser:**
        *   Create a function `parseAndExecute(scriptPath)` in `HidManager`.
        *   It will use `SdCardManager::getInstance().openLineReader` to read a script file (e.g., from `/user/hid_scripts/`) line by line.
        *   Implement a parser for basic commands: `STRING`, `DELAY`, `ENTER`, `GUI` (Windows key), `COMMAND` (macOS key), etc.
        *   The parser translates these commands into `TinyUSB_Keyboard` functions like `keyboard.press()`, `keyboard.release()`, `keyboard.print()`.
    4.  **Core Logic:**
        *   The `HidManager` needs to initialize the USB stack as a keyboard device.
        *   The menu will "arm" a script. The `HidManager` will then wait for a USB connection. Upon connection, it triggers `parseAndExecute()`.
    5.  **Menu Flow:**
        *   `Tools -> HID Attacks` (new Grid Menu).
        *   `BadUSB -> [ListMenu of scripts from SD]` -> Select a script.
        *   The screen changes to "ARMED. Plug into target computer."

#### 8. Bad Bluetooth Keyboard & Mouse
*   **Concept:** The wireless version of BadUSB. The KIVA advertises as a Bluetooth keyboard or mouse, waits for a victim to pair with it, and then executes a Ducky Script (for the keyboard) or causes chaotic movement (for the mouse).
*   **Implementation Plan:**
    1.  **Library:** `NimBLE-Arduino` has built-in support for BLE HID profiles.
    2.  **Enhance `HidManager`:** Add modes for Bluetooth operation.
    3.  **Core Logic (Keyboard):**
        *   Use `NimBLEDevice::createServer()` and create a `NimBLEHIDDevice`.
        *   Set up the HID Report Map to describe a keyboard.
        *   Advertise as a keyboard device.
        *   Once a host connects, you can use `hid.inputKeyboard->send()` to send keystrokes, using the same Ducky Script parser as BadUSB.
    4.  **Core Logic (Mouse):**
        *   Similar setup, but use a HID Report Map for a mouse.
        *   Once connected, the `loop()` function will simply call `hid.inputMouse->move(random_dx, random_dy)` to create chaotic cursor movement.
    5.  **Menu Flow:**
        *   `HID Attacks -> Bad BT Keyboard -> [List of scripts]` -> "ADVERTISING... Waiting for host to pair."
        *   `HID Attacks -> Jitter Mouse` -> "ADVERTISING... Waiting for host to pair."

---
### Category 5: Network Service Spoofing

#### 9. MDNS Rick Roll
*   **Method Name:** `MdnsSpoofer`
*   **Concept:** Disrupt local network services by broadcasting malicious Multicast DNS (MDNS/Bonjour) packets. This attack makes fake services (like "Rick Astley's iPhone" or a fake printer) appear on all devices on the same network, causing confusion and pop-ups.
*   **Implementation Plan:**
    1.  **Library:** Use the built-in `ESPmDNS` library.
    2.  **Create Manager Class:** `MdnsSpoofer.h` and `.cpp`.
    3.  **Core Logic:**
        *   The KIVA device must first connect to the target WiFi network as a normal client.
        *   The `MdnsSpoofer::start()` method will run a loop or task.
        *   Inside the loop, it will repeatedly announce fake services. Example: `MDNS.addService("rick_iphone", "_apple-mobdev2", "tcp", 62078);`. You can add multiple fake services for printers (`_ipp._tcp`), etc.
        *   To be more effective, you can have it find the router's IP and announce the services as coming from the router, making them seem more legitimate.
    4.  **Menu Flow:**
        *   `WiFi Tools -> Network Pranks -> MDNS Spoofer`.
        *   The menu must first check if the device is connected to WiFi. If not, it should prompt the user to connect via the `WifiListMenu`.
        *   Once connected, an active screen shows "Spoofing services on [SSID]...".

---
### Category 6: Physical Hardware Attacks

#### 10. IR Blaster / Universal Remote
*   **Concept:** Use an Infrared LED to send commands to TVs, AC units, projectors, and other IR-controlled devices, effectively turning the KIVA into a universal remote or a "TV-B-Gone".
*   **Implementation Plan:**
    1.  **Hardware:** Connect an IR LED (and a current-limiting resistor) to a free GPIO pin.
    2.  **Library:** Add `IRremoteESP8266` to your `platformio.ini`.
    3.  **Create Manager Class:** `IrManager.h` and `.cpp`.
    4.  **IR Code Storage:** Store IR codes on the SD card in a simple, structured format (e.g., JSON or a custom format) in a file like `/user/ir_codes.json`. This file would map device names to commands and their corresponding raw or protocol-specific codes.
        ```json
        { "TVs": [ { "name": "Samsung Power", "protocol": "SAMSUNG", "code": "0xE0E040BF" }, ... ],
          "Projectors": [ ... ] }
        ```
    5.  **Core Logic:**
        *   The `IrManager` will load and parse this JSON file at startup.
        *   The `send(deviceName, commandName)` method will look up the code and use the library's sender object (`IRsend irsend(PIN);`) to transmit it, e.g., `irsend.sendSAMSUNG(code, 32);`.
        *   A "Chaos Mode" (TV-B-Gone) would simply iterate through a list of common power-off codes for many brands and send them in a rapid sequence.
    6.  **Menu Flow:**
        *   `Utilities -> IR Blaster`.
        *   This leads to a `ListMenu` showing device categories from the JSON file.
        *   Selecting a category leads to another `ListMenu` showing commands for that device.
        *   Selecting a command calls `irManager.send()` and shows a confirmation like "Sent!".
