#ifndef SYSTEM_DATA_PROVIDER_H
#define SYSTEM_DATA_PROVIDER_H

#include <cstdint>
#include <string>

class App;

struct MemoryUsage {
    uint64_t used;
    uint64_t total;
    uint8_t percentage;
};

class SystemDataProvider {
public:
    SystemDataProvider();
    void setup(App* app);
    void update();

    const MemoryUsage& getRamUsage() const;
    const MemoryUsage& getPsramUsage() const;
    const MemoryUsage& getSdCardUsage() const;
    uint32_t getCpuFrequency() const;
    float getTemperature() const;

    static std::string formatBytes(uint64_t bytes);

private:
    void updateData();

    App* app_;
    unsigned long lastUpdateTime_;

    MemoryUsage ramUsage_;
    MemoryUsage psramUsage_;
    MemoryUsage sdCardUsage_;
    uint32_t cpuFrequency_;
    float temperature_;
};

#endif // SYSTEM_DATA_PROVIDER_H
