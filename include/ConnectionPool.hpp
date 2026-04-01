#pragma once

#include <iostream>
#include <vector>

struct ConnectionPool {
  // {serverId -> {connection_number}} = [serverId][Connection_index] ->
  // direct_fd from registered files
  std::vector<std::vector<int>> serverConnectionPool;

  void init(int serverCount = 3, int connectionCount = 50) {
    std::cout << "----Initializing Connection Pool----" << std::endl;
    for (size_t server = 0; server < serverCount; server++) {
      std::vector<int> temp;
      temp.reserve(connectionCount); // reserve space for storing direct_fd of
                                     // 50 connections for each backend server
      serverConnectionPool.push_back(temp);
    }
    std::cout << "----Completed Init of Connection Pool----" << std::endl;
  }

  int getServerConnection(int serverId) {
    size_t size = serverConnectionPool[serverId].size();
    if (size <= 0)
      return -1; // no connection available

    // get the last element because it was already in cache hence faster to work
    int connectionId = serverConnectionPool[serverId][size - 1];

    serverConnectionPool[serverId].pop_back();
    return connectionId;
  }

  void returnConnection(int serverId, int connectionId) {
    serverConnectionPool[serverId].push_back(connectionId);
  }
};
