#pragma once

#include <map>
#include <memory>
#include <string>
#include "Service.h"

class App;

class ServiceManager {
public:
    ServiceManager(App* app);
    ~ServiceManager();

    template <typename T>
    T* getService() {
        size_t id = Service::getServiceId<T>();
        if (services_.find(id) == services_.end()) {
            services_[id] = std::make_unique<T>();
            services_[id]->setup(app_);
        }
        return static_cast<T*>(services_[id].get());
    }

    void loop();
    void destroyAllServices();

    uint32_t getTotalResourceRequirements() const;

private:
    App* app_;
    std::map<size_t, std::unique_ptr<Service>> services_;
};
