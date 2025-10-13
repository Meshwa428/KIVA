
# Getting Started

This guide provides instructions on how to set up your development environment, build the KIVA firmware, and flash it to your device.

## Prerequisites

-   [PlatformIO](https.platformio.org/): A professional collaborative platform for embedded development.
-   A text editor or IDE, such as [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO IDE extension](https://marketplace.visualstudio.com/items?itemName=platformio.platformio-ide).

## Building and Uploading

This is a standard PlatformIO project. All dependencies are defined in the `platformio.ini` file and will be installed automatically.

1.  **Build the project:**

    ```bash
    pio run
    ```

2.  **Upload to the device:**

    ```bash
    pio run -t upload
    ```

3.  **Monitor serial output:**

    ```bash
    pio device monitor
    ```
