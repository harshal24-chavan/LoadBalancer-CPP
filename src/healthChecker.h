
#include "loadbalancer.h"
#include "tomlParser.h"

class HealthChecker {
private:
  LoadBalancer &loadbalancer;
  AppConfig config;
  void monitoringLoop();

public:
  HealthChecker(LoadBalancer &lb, AppConfig conf);
  void startMonitoring();
};
