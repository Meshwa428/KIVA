#include "ServiceManager.h"
#include "App.h"

ServiceManager::ServiceManager(App* app) : app_(app) {}

ServiceManager::~ServiceManager() {
    destroyAllServices();
}

void ServiceManager::loop() {
    for (auto const& [typeIndex, service] : services_) {
        service->loop();
    }
}

void ServiceManager::destroyAllServices() {
    services_.clear();
}
