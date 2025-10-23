
#include "loadbalancer.h"

class HealthChecker {
private:
  LoadBalancer &loadbalancer;
  void monitoringLoop();

public:
  HealthChecker(LoadBalancer &lb);
  void startMonitoring();
};
