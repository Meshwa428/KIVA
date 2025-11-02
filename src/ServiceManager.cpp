#include "ServiceManager.h"
#include "App.h"

ServiceManager::ServiceManager(App* app) : app_(app) {}

ServiceManager::~ServiceManager() {
    destroyAllServices();
}

uint32_t ServiceManager::getTotalResourceRequirements() const {
    uint32_t totalRequirements = (uint32_t)ResourceRequirement::NONE;
    for (auto const& [typeIndex, service] : services_) {
        totalRequirements |= service->getResourceRequirements();
    }
    return totalRequirements;
}

void ServiceManager::loop() {
    for (auto const& [typeIndex, service] : services_) {
        service->loop();
    }
}

void ServiceManager::destroyAllServices() {
    services_.clear();
}
