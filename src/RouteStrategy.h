// routestrategy.h

#pragma once // Important: Prevents the header from being included multiple
             // times

#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Forward declare Server so we don't need to include its full definition here
class Server;

// The abstract interface (the "contract")
class IRouteStrategy {
public:
  virtual ~IRouteStrategy() = default;
  virtual Server &
  selectServer(const std::vector<std::shared_ptr<Server>> &serverList) = 0;
};

// The concrete strategy declaration
class RoundRobin : public IRouteStrategy {
private:
  size_t serverCount = 0;
  // Note: The mutex is an implementation detail, so it can be omitted here
  // or you can include <mutex> and declare it.
  mutable std::mutex mtx;

public:
  // The constructor declaration
  RoundRobin() = default;

  // The method declaration
  Server &
  selectServer(const std::vector<std::shared_ptr<Server>> &serverList) override;
};

// You can declare other strategies here as well
// class LeastConnection : public IRouteStrategy { ... };
//

// The concrete strategy declaration
class LeastConnection : public IRouteStrategy {
public:
  // The constructor declaration
  LeastConnection() = default;
  mutable std::mutex mtx;

  // The method declaration
  Server &
  selectServer(const std::vector<std::shared_ptr<Server>> &serverList) override;
};

class StrategyFactory {
public:
  static std::unique_ptr<IRouteStrategy>
  getStrategy(const std::string &strategy);
};
