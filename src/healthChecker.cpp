#include<thread>
#include<vector>
#include<string>
#include<chrono>
#include<cpr/cpr.h>

#include "loadbalancer.h"

class HealthChecker{
private:
  LoadBalancer& loadbalancer;

  void checkServerHealth(std::vector<Server>& serverList){
    int ind = 0;
    int retryCount = 0;
    while(true){
      for(auto& server : serverList){
        try{
          Server& server = serverList[ind];
          ind = (ind + 1) % serverList.size();

          std::string fullUrl = server.getUrl() + "/health";

          cpr::Timeout timeout = cpr::Timeout{3000};// get from appconfig
          cpr::Response cprRes = cpr::Get(cpr::Url{fullUrl}, timeout, cpr::ConnectTimeout{30000});

          if(cprRes.status_code == 200){
            server.markHealthy();
          }
        }catch(const std::exception& e){
          retryCount++;
          if(retryCount > 3){
            server.markUnhealthy();
          }
        }
      }
      // after checking all servers, sleep
      std::this_thread::sleep_for(std::chrono::seconds(5));
    }
  }
public:
  HealthChecker(LoadBalancer& lb) : loadbalancer(lb){}

  void startMonitoring(){

    std::vector<std::unique_ptr<Server>> serverList = loadbalancer.getServerList();
    std::thread monitorThread(&checkServerHealth, serverList);

    monitorThread.join();
  }
};
