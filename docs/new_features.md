# KIVA Project: Feature & Settings Roadmap

This document outlines the implemented, planned, and proposed features for the KIVA device, focusing on user settings, quality of life improvements, and advanced hardware control.

## Phase 1: Core Settings Framework (Implemented)

This foundational phase is complete, providing a robust system for user customization.

- **✅ Centralized Settings Management (`ConfigManager`)**

  - **Description:** A unified system for managing all device settings.
  - **Persistence:** Uses a hybrid model, loading from fast EEPROM on boot, with a human-readable JSON file (`/config/settings.json`) on the SD card as a backup. Automatically creates default settings if none exist.
  - **Benefit:** Ensures user preferences are saved instantly and persist across reboots for a reliable and consistent experience.

- **✅ Redesigned Settings Menu**

  - **Description:** A scalable, list-based menu for all configuration options.
  - **Benefit:** Provides a clean, intuitive, and extensible interface for all current and future settings.

- **✅ Unified & Individual Brightness Control**

  - **Description:**
    1.  A main "Brightness" slider in the settings menu controls both displays simultaneously for quick adjustments.
    2.  An advanced "Twin Dial" UI, accessible via the `A` button, allows for independent control of the Main and Auxiliary display brightness.
  - **Benefit:** Offers both simplicity for common use and granular control for power users, allowing them to optimize for stealth or battery life.

- **✅ Persistent Keyboard Layout Selection**

  - **Description:** A global setting to choose the keyboard layout (e.g., US, UK, DE) for all HID-related functions.
  - **Benefit:** Critical for international users and ensures Ducky Scripts execute correctly on various target systems without needing to modify script files.

- **✅ Configurable OTA Password**
  - **Description:** The password for both Web and IDE OTA updates is now a persistent setting, editable via the on-screen keyboard.
  - **Benefit:** Enhances security by allowing the user to change the default password and simplifies management by removing the need to edit SD card files.

## Phase 2: High-Impact Features (Next to Implement)

These features will add significant new capabilities, transforming the device from a simple tool into a versatile platform.

- **🔄 Screen Timeout (Sleep Mode)**

  - **Plan:** Implement an inactivity timer that puts both OLED displays into a low-power sleep state. Any button press will wake them instantly.
  - **Benefit:** The single most effective feature for **dramatically increasing battery life**. Also enhances stealth by automatically blanking the screens.

- **🔄 Headless Mode ("Arm for Boot")**

  - **Plan:** Create a setting to pre-configure an action (e.g., Run Ducky Script, Start Beacon Spam) to execute immediately on boot, bypassing the UI entirely.
  - **Benefit:** Transforms the KIVA into a **deployable hardware payload**. A user can "arm" the device, plug it into a target, and have it execute its mission autonomously upon receiving power.

- **🔄 Profiles Manager**

  - **Plan:** A system to save and load the entire settings configuration as named profiles (e.g., "Stealth", "Presentation", "Attack").
  - **Benefit:** A massive efficiency boost for power users, allowing them to switch between entire device configurations for different operational scenarios in seconds.

- **🔄 Customizable BLE Keymapper**
  - **Plan:** A dedicated mode where the physical navigation buttons are mapped to custom keyboard presses or shortcuts, sent over BLE. A new settings screen will allow users to configure these mappings.
  - **Benefit:** Turns the device into a powerful, customizable **BLE remote control** for presentations, media playback, or triggering complex macros on a host computer.

## Phase 3: Quality of Life & Advanced Customization

These features will refine the user experience, add personalization, and provide deeper control for advanced users.

- **✨ Persistent Vibration Setting**

  - **Plan:** A global toggle in settings to enable or disable all haptic feedback.
  - **Benefit:** Provides full control over the device's physical feedback, essential for situations requiring complete silence.

- **✨ Configurable BLE Device Name**

  - **Plan:** An option to change the advertised Bluetooth name of the device using the on-screen keyboard.
  - **Benefit:** Enhances **social engineering and stealth** by allowing the device to masquerade as an innocuous peripheral like "Logitech Mouse" or "JBL Speaker".

- **✨ Status Bar Customization**

  - **Plan:** A new settings submenu with toggles to show/hide elements like the battery percentage, voltage, etc.
  - **Benefit:** Allows users to **personalize their workspace** by de-cluttering the UI to show only the information they deem critical.

- **✨ Encoder Direction Flip**

  - **Plan:** A simple toggle to reverse the scrolling direction of the rotary encoder.
  - **Benefit:** An important **ergonomic and accessibility feature** that makes the device feel more intuitive for all users, regardless of their preference.

- **✨ Boot Animation Toggle**

  - **Plan:** An option to disable the boot progress bar animation.
  - **Benefit:** Provides a **faster boot time** for experienced users who prefer to get to the main menu as quickly as possible.

- **✨ Factory Reset with Backup**

  - **Plan:** A "Factory Reset" option that, after confirmation, moves all configuration files to a timestamped backup folder, wipes the EEPROM, and reboots the device.
  - **Benefit:** A critical **safety and recovery mechanism**. The backup feature is a crucial safeguard against accidental data loss.

- **✨ Log & Capture Rotation (Auto-Cleanup)**
  - **Plan:** A setting to automatically keep only the X most recent log and capture files, deleting the oldest ones to free up space.
  - **Benefit:** "Set and forget" storage management that **prevents the SD card from filling up**, ensuring the device is always ready to capture new data.
