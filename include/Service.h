#pragma once
#include <cstddef>
#include "Resource.h"

class App;

class Service {
private:
    static size_t nextServiceId_;

public:
    virtual ~Service() = default;
    virtual void setup(App* app) = 0;
    virtual void loop() {};

    /**
     * @brief Declares the hardware resources this service currently requires.
     * @return A bitmask of ResourceRequirement flags.
     */
    virtual uint32_t getResourceRequirements() const { return (uint32_t)ResourceRequirement::NONE; }

    template <typename T>
    static size_t getServiceId() {
        static size_t id = nextServiceId_++;
        return id;
    }
};
