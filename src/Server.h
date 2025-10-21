#pragma once // Prevents the file from being included multiple times
//
#include<atomic>
#include<string>

class Server{
private:
  std::atomic<int> activeConnections{0};
  std::string url = "";
  std::atomic<bool> healthy = true;
public:
  Server(std::string url);
  const std::string getUrl() const;

  bool checkHealth() const ;
  void markUnhealthy();
  void markHealthy();

  void incrementActiveConnection();
  void decrementActiveConnection();

  int getConnections() const;
};
