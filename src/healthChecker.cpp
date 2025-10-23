#include <chrono>
#include <cpr/cpr.h>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "healthChecker.h"
#include "loadbalancer.h"

void HealthChecker::monitoringLoop() {
  while (true) {
    std::cout << "[HealthCheck] Running health checks..." << std::endl;

    loadbalancer.accept([this](Server &server) {
      auto timeout = cpr::Timeout{5000}; // 30 second timeout
      cpr::Response res =
          cpr::Head(cpr::Url{server.getUrl() + "/health"}, timeout);

      bool current_status = false;
      if (res.status_code >= 200 && res.status_code < 400) {
        current_status = true;
      }
      if (server.checkHealth() != current_status) {
        std::cout << "  [HealthCheck] Server " << server.getUrl()
                  << " changed state to "
                  << (current_status ? "Healthy" : "Unhealthy") << std::endl;

        // add or remove the server from acitveServerList
        if (current_status == false) {
          // unhealthy server
          loadbalancer.removeActiveServer(server.getUrl());
        } else {
          loadbalancer.addActiveServer(server.getUrl());
        }
      }
      server.setHealth(current_status);
    });

    std::this_thread::sleep_for(std::chrono::seconds(10));
  }
}

HealthChecker::HealthChecker(LoadBalancer &lb) : loadbalancer(lb) {}

void HealthChecker::startMonitoring() {

  std::thread([this]() { this->monitoringLoop(); }).detach();
}
