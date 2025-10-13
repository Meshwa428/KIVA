
# KIVA Development Roadmap

This document outlines the planned features and improvements for the KIVA project.

## High-Impact Features

These features are the next priority and will add significant new capabilities to the device.

-   **Screen Timeout (Sleep Mode):** Implement an inactivity timer to put the OLED displays to sleep, dramatically increasing battery life.
-   **Headless Mode ("Arm for Boot"):** Pre-configure an action (e.g., Run Ducky Script, Start Beacon Spam) to execute immediately on boot, turning KIVA into a deployable hardware payload.
-   **Profiles Manager:** A system to save and load entire device configurations for different operational scenarios.
-   **Customizable BLE Keymapper:** A dedicated mode to map the physical navigation buttons to custom keyboard presses or shortcuts, turning the device into a powerful BLE remote control.

## Quality of Life & Advanced Customization

These features will refine the user experience and provide deeper control for advanced users.

-   **Persistent Vibration Setting:** A global toggle to enable or disable all haptic feedback.
-   **Configurable BLE Device Name:** An option to change the advertised Bluetooth name of the device for social engineering and stealth.
-   **Status Bar Customization:** A new settings submenu with toggles to show/hide elements like the battery percentage, voltage, etc.
-   **Encoder Direction Flip:** A simple toggle to reverse the scrolling direction of the rotary encoder.
-   **Boot Animation Toggle:** An option to disable the boot progress bar animation for a faster boot time.
-   **Factory Reset with Backup:** A "Factory Reset" option that backs up configuration files before wiping the device.
-   **Log & Capture Rotation (Auto-Cleanup):** A setting to automatically keep only the X most recent log and capture files, deleting the oldest ones to free up space.
