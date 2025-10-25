#include "MetricsCalc.h"
#include <atomic>

MetricsCalc &MetricsCalc::getInstance() {
  static MetricsCalc mc;
  return mc;
}

void MetricsCalc::incrementConnections() { activeConnections++; }
void MetricsCalc::decreaseConnections() { activeConnections--; }

void MetricsCalc::incrementRequestCount() { totalRequests++; }

void MetricsCalc::setTotalResponseTimeMS(long long duration) {
  totalResponseTimeMS += duration;
  totalResponses++;
}

void MetricsCalc::incrementRps() { rpsCurrentSecond++; }
void MetricsCalc::rotateRps() {
  rpsLastSecond.store(rpsCurrentSecond.load());
  rpsCurrentSecond.store(0);
}

long long MetricsCalc::getActiveConnections() const {
  return activeConnections.load();
}
long long MetricsCalc::getTotalRequests() const { return totalRequests.load(); }

long long MetricsCalc::getTotalResponses() const {
  return totalResponses.load();
}
long long MetricsCalc::getTotalResponseTimeMS() const {
  return totalResponseTimeMS.load();
}
long long MetricsCalc::getRps() const { return rpsLastSecond.load(); }

long long MetricsCalc::getAvgResponseTime() {
  auto duration = MetricsCalc::getInstance().getTotalResponseTimeMS();
  auto requests = MetricsCalc::getInstance().getTotalRequests();
  if (requests == 0)
    return 0;
  long long avgResponseTime = duration / requests;
  return avgResponseTime;
}
