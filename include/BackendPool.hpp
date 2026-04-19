#pragma once

#include <iostream>
#include <vector>

struct BackendPool {
  // {serverId -> {connection_number}} = [serverId][Connection_index] ->
  // direct_fd from registered files
  std::vector<std::vector<int>> serverBackendPool;

  void init(int serverCount = 3, int connectionCount = 50) {
    std::cout << "----Initializing Connection Pool----" << std::endl;
    for (size_t server = 0; server < serverCount; server++) {
      std::vector<int> temp;
      temp.reserve(connectionCount); // reserve space for storing direct_fd of
                                     // 50 connections for each backend server
      serverBackendPool.push_back(temp);
    }
    std::cout << "----Completed Init of Connection Pool----" << std::endl;
  }

  int getServerConnection(int serverId) {
    size_t size = serverBackendPool[serverId].size();
    if (size <= 0)
      return -1; // no connection available

    // get the last element because it was already in cache hence faster to work
    int connectionId = serverBackendPool[serverId][size - 1];

    serverBackendPool[serverId].pop_back();
    return connectionId;
  }

  int getServerCount() { return serverBackendPool.size(); }

  int getServerConnectionCount(int serverId) {
    return serverBackendPool[serverId].size();
  }

  void returnConnection(int serverId, int connectionId) {
    serverBackendPool[serverId].push_back(connectionId);
  }
};
