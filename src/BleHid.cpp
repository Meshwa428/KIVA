#include "BadUSB.h"
#include <NimBLEHIDDevice.h>

// --- ASCII to HID Scancode Lookup Table ---
#define SHIFT 0x80
const uint8_t _asciimap[128] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2a, 0x2b, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2c, 0x1e|SHIFT, 0x34|SHIFT, 0x20|SHIFT, 0x21|SHIFT, 0x22|SHIFT, 0x24|SHIFT, 0x34,
    0x26|SHIFT, 0x27|SHIFT, 0x25|SHIFT, 0x2e|SHIFT, 0x36, 0x2d, 0x37, 0x38,
    0x27, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26,
    0x33|SHIFT, 0x33, 0x36|SHIFT, 0x2e, 0x37|SHIFT, 0x38|SHIFT, 0x1f|SHIFT,
    0x04|SHIFT, 0x05|SHIFT, 0x06|SHIFT, 0x07|SHIFT, 0x08|SHIFT, 0x09|SHIFT, 0x0a|SHIFT, 0x0b|SHIFT,
    0x0c|SHIFT, 0x0d|SHIFT, 0x0e|SHIFT, 0x0f|SHIFT, 0x10|SHIFT, 0x11|SHIFT, 0x12|SHIFT, 0x13|SHIFT,
    0x14|SHIFT, 0x15|SHIFT, 0x16|SHIFT, 0x17|SHIFT, 0x18|SHIFT, 0x19|SHIFT, 0x1a|SHIFT, 0x1b|SHIFT,
    0x1c|SHIFT, 0x1d|SHIFT, 0x2f, 0x31, 0x30, 0x23|SHIFT, 0x2d|SHIFT, 0x35,
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
    0x2f|SHIFT, 0x31|SHIFT, 0x30|SHIFT, 0x35|SHIFT, 0
};

// --- Standard BLE HID Report Descriptor for a Keyboard ---
static const uint8_t hidReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,
  USAGE(1),           0x06,
  COLLECTION(1),      0x01,
  REPORT_ID(1),       0x01,
  USAGE_PAGE(1),      0x07,
  USAGE_MINIMUM(1),   0xE0,
  USAGE_MAXIMUM(1),   0xE7,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x01,
  REPORT_SIZE(1),     0x01,
  REPORT_COUNT(1),    0x08,
  HIDINPUT(1),        0x02,
  REPORT_COUNT(1),    0x01,
  REPORT_SIZE(1),     0x08,
  HIDINPUT(1),        0x01,
  REPORT_COUNT(1),    0x05,
  REPORT_SIZE(1),     0x01,
  USAGE_PAGE(1),      0x08,
  USAGE_MINIMUM(1),   0x01,
  USAGE_MAXIMUM(1),   0x05,
  HIDOUTPUT(1),       0x02,
  REPORT_COUNT(1),    0x01,
  REPORT_SIZE(1),     0x03,
  HIDOUTPUT(1),       0x01,
  REPORT_COUNT(1),    0x06,
  REPORT_SIZE(1),     0x08,
  LOGICAL_MINIMUM(1), 0x00,
  LOGICAL_MAXIMUM(1), 0x65,
  USAGE_PAGE(1),      0x07,
  USAGE_MINIMUM(1),   0x00,
  USAGE_MAXIMUM(1),   0x65,
  HIDINPUT(1),        0x00,
  END_COLLECTION(0)
};

// --- BleHid Implementation (NimBLE-based) ---
BleHid::BleHid(const std::string& deviceName) :
    deviceName_(deviceName), hid_(nullptr), inputKeyboard_(nullptr), pServer_(nullptr), connected_(false) {
    memset(_keyReport, 0, sizeof(_keyReport));
}

bool BleHid::begin() {
    // Clean up any existing instance first
    if (NimBLEDevice::getInitialized()) {
        end();
        delay(500); // Give time for cleanup
    }

    NimBLEDevice::init(deviceName_);
    pServer_ = NimBLEDevice::createServer();
    pServer_->setCallbacks(this);

    hid_ = new NimBLEHIDDevice(pServer_);
    inputKeyboard_ = hid_->inputReport(1);

    hid_->manufacturer()->setValue("Kiva");
    hid_->pnp(0x02, 0x05ac, 0x820a, 0x0210);
    hid_->hidInfo(0x00, 0x01);

    NimBLEDevice::setSecurityAuth(true, true, true);

    hid_->reportMap((uint8_t*)hidReportDescriptor, sizeof(hidReportDescriptor));
    hid_->startServices();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(hid_->hidService()->getUUID());
    pAdvertising->start();

    return true;
}

void BleHid::end() {
    // Clear connection state first
    connected_ = false;
    
    // Stop advertising
    if (NimBLEDevice::getInitialized()) {
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        if (pAdvertising->isAdvertising()) {
            pAdvertising->stop();
        }
    }

    // Disconnect all clients gracefully
    if (pServer_ && pServer_->getConnectedCount() > 0) {
        std::vector<uint16_t> clients = pServer_->getPeerDevices();
        for (auto& client : clients) {
            pServer_->disconnect(client);
        }
        delay(100); // Allow disconnection to complete
    }

    // Clean up HID device and input report references
    inputKeyboard_ = nullptr; // Don't delete, managed by NimBLEHIDDevice
    
    if (hid_) {
        delete hid_;
        hid_ = nullptr;
    }

    // Clean up server reference
    pServer_ = nullptr; // Don't delete, managed by NimBLEDevice

    // Give time for cleanup
    delay(100);

    // Deinitialize NimBLE stack
    if (NimBLEDevice::getInitialized()) {
        NimBLEDevice::deinit(true);
    }
    
    delay(100);
}


void BleHid::onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
    connected_ = true;
    pServer->updateConnParams(desc->conn_handle, 12, 12, 0, 60);
}

void BleHid::onDisconnect(NimBLEServer* pServer) {
    connected_ = false;
    // Only restart advertising if we're still initialized and not shutting down
    if (NimBLEDevice::getInitialized() && hid_ != nullptr) {
        delay(500); // Wait before restarting advertising
        NimBLEDevice::getAdvertising()->start();
    }
}

bool BleHid::isConnected() {
    return connected_ && pServer_ && pServer_->getConnectedCount() > 0;
}

size_t BleHid::press(uint8_t k) {
    if (!isConnected()) return 0;

    if (k >= 136) { // Non-printing key
        k -= 136;
    } else if (k >= 128) { // Modifier
        _keyReport[0] |= (1 << (k - 128));
        k = 0;
    } else { // Printing key
        k = pgm_read_byte(_asciimap + k);
        if (!k) return 0;
        if (k & SHIFT) {
            _keyReport[0] |= 0x02; // Left Shift
            k &= 0x7F;
        }
    }

    // Add key to report if not already present
    if (_keyReport[2] != k && _keyReport[3] != k &&
        _keyReport[4] != k && _keyReport[5] != k &&
        _keyReport[6] != k && _keyReport[7] != k) {

        for (int i = 2; i < 8; i++) {
            if (_keyReport[i] == 0) {
                _keyReport[i] = k;
                break;
            }
        }
    }
    sendReport();
    return 1;
}

size_t BleHid::release(uint8_t k) {
    if (!isConnected()) return 0;

    if (k >= 136) {
        k -= 136;
    } else if (k >= 128) {
        _keyReport[0] &= ~(1 << (k - 128));
        k = 0;
    } else {
        k = pgm_read_byte(_asciimap + k);
        if (!k) return 0;
        if (k & SHIFT) {
            _keyReport[0] &= ~0x02;
            k &= 0x7F;
        }
    }

    for (int i = 2; i < 8; i++) {
        if (_keyReport[i] == k) {
            _keyReport[i] = 0;
        }
    }
    sendReport();
    return 1;
}

void BleHid::releaseAll() {
    memset(_keyReport, 0, sizeof(_keyReport));
    sendReport();
}

size_t BleHid::write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    for (size_t i = 0; i < size; i++) {
        if (press(buffer[i])) {
            release(buffer[i]);
            n++;
        } else {
            break;
        }
    }
    return n;
}

void BleHid::sendReport() {
    if (isConnected() && inputKeyboard_) {
        try {
            inputKeyboard_->setValue(_keyReport, sizeof(_keyReport));
            inputKeyboard_->notify();
            delay(10);
        } catch (...) {
            // Ignore exceptions during shutdown
        }
    }
}