#pragma once

#include "crow.h"
#include<cpr/cpr.h>

#include "loadbalancer.h"

class RequestForwarder{
private:
    LoadBalancer& lb;
    cpr::Header convertHeaders(const crow::request& req);
    crow::response convertResponse(const cpr::Response& cprRes);
public:
    RequestForwarder(LoadBalancer& loadbalancer);
    crow::response forward(const crow::request& req, int maxRetries = 1);
};
