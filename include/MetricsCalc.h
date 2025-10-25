#pragma once

#include <atomic>

class MetricsCalc {
private:
  std::atomic<long long> activeConnections{0};
  std::atomic<long long> totalResponseTimeMS{0};

  std::atomic<long long> totalRequests{0};
  std::atomic<long long> totalResponses{0};

  std::atomic<long long> rpsCurrentSecond{0};
  std::atomic<long long> rpsLastSecond{0};

  MetricsCalc() = default;
  // deleting the copy constructor
  MetricsCalc(const MetricsCalc &) = delete;
  MetricsCalc &operator=(const MetricsCalc &) = delete;

public:
  static MetricsCalc &getInstance();

  int getConnectionsCount();
  void incrementConnections();
  void decreaseConnections();

  void incrementRequestCount();

  void setTotalResponseTimeMS(long long duration);

  void incrementRps();
  void rotateRps();

  long long getActiveConnections() const;
  long long getTotalRequests() const;
  long long getTotalResponses() const;
  long long getTotalResponseTimeMS() const;
  long long getRps() const;
  long long getAvgResponseTime();
};
