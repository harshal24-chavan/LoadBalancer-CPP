#pragma once

#include "Server.h"
#include "crow.h"
#include "loadbalancer.h"
#include <cpr/cpr.h>

class IRequestHandler {
public:
  // IRequestHandler(LoadBalancer &lb);
  ~IRequestHandler() = default;
  virtual crow::response handle(const crow::request &req) = 0;
};

class ProxyHandler : public IRequestHandler {
private:
  LoadBalancer &lb;

  cpr::Header convertHeaders(const crow::request &req);
  crow::response convertResponse(const cpr::Response &cprRes);

public:
  ProxyHandler(LoadBalancer &loadbalancer);
  crow::response handle(const crow::request &req);
};
