#include "crow.h"
#include<cpr/cpr.h>

#include<iostream>
#include<string>
#include<vector>
#include<atomic>
#include<mutex>
#include<chrono>

class RequestForwarder{
private:
    LoadBalancer &lb;
};
