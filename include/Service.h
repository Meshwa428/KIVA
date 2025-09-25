#pragma once
#include <cstddef>

class App;

class Service {
private:
    static size_t nextServiceId_;

public:
    virtual ~Service() = default;
    virtual void setup(App* app) = 0;
    virtual void loop() {};

    template <typename T>
    static size_t getServiceId() {
        static size_t id = nextServiceId_++;
        return id;
    }
};
