#include "crow.h"
#include<iostream>

#include<atomic>
#include<memory>
#include<utility>

#include<string>
#include<vector>

#include "RouteStrategy.h"
#include "Server.h"
#include "tomlParser.h"



// load balancer singleton
class LoadBalancer{
private:
    std::vector<std::unique_ptr<Server>> serverList;
    std::unique_ptr<IRouteStrategy> routeStrategy;

    LoadBalancer() = default;

    // deleting the copy constructor
    LoadBalancer(const LoadBalancer&) = delete;
    LoadBalancer& operator =(const LoadBalancer&) = delete;

public:
    static LoadBalancer& getInstance(){
        static LoadBalancer lb;
        return lb;
    }
    void addServer(std::string url){
        serverList.push_back(std::make_unique<Server>(url));
    }

    void removeServer(std::string url){
        std::erase_if(serverList, [&](const std::unique_ptr<Server>& server){
            return server->getUrl() == url;
        });
    }
    void listServers(){
        std::cout<<"------------Registered Servers------------"<<std::endl;

        for(const auto& server: serverList){
            std::cout<<server->getUrl()<<std::endl;
        }
        
        std::cout<<"------------------------------------------"<<std::endl;
    }

    void setStrategy(std::unique_ptr<IRouteStrategy> strategy) {
        routeStrategy = std::move(strategy);
    }

    Server& routeRequest(){
        Server& selectedServer = routeStrategy->selectServer(serverList);
    
        // make http request using crp:
        return selectedServer;
    }
};


int main(){
    AppConfig config = parseTomlFile("config.toml");

    crow::SimpleApp app;

    LoadBalancer& lb = LoadBalancer::getInstance();
    lb.addServer("www.google.com");
    lb.addServer("www.facebook.com");
    lb.addServer("www.x.com");
    lb.listServers();

    lb.setStrategy(StrategyFactory::getStrategy(config.strategy));



// 3. 🌐 Define the main endpoint
    // This endpoint will handle all incoming requests to be balanced.
    CROW_ROUTE(app, "/")
    .methods("GET"_method, "POST"_method)
    ([](){
        try {
            // Get the singleton instance inside the handler
            LoadBalancer& lb_instance = LoadBalancer::getInstance();
            
            // Delegate the work to the load balancer
            Server& chosen_server = lb_instance.routeRequest();
            
            // Prepare a success response (JSON is a good choice)
            crow::json::wvalue response_json;
            response_json["status"] = "success";
            response_json["routed_to"] = chosen_server.getUrl();
            
            std::cout << "Request routed to: " << chosen_server.getUrl() << std::endl;
            return crow::response(200, response_json);

        } catch (const std::exception& e) {
            // Handle errors gracefully (e.g., no servers available)
            crow::json::wvalue error_json;
            error_json["status"] = "error";
            error_json["message"] = e.what();

            std::cerr << "Error handling request: " << e.what() << std::endl;
            // Return a 500 Internal Server Error status code
            return crow::response(500, error_json);
        }
    });

    // 4. ▶️ Start the server
    std::cout << "🔥 Load Balancer server starting on port " << config.port << std::endl;
    app.port(config.port).multithreaded().run();}
