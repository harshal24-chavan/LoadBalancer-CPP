#include "crow.h"

#include <cpr/cpr.h>

#include <iostream>

#include <atomic>
#include <memory>
#include <utility>

#include <map>
#include <string>
#include <vector>

#include "RouteStrategy.h"
#include "Server.h"
#include "tomlParser.h"

// load balancer singleton
class LoadBalancer {
private:
    std::vector<std::unique_ptr<Server>> serverList;
    std::unique_ptr<IRouteStrategy> routeStrategy;

    LoadBalancer() = default;

    // deleting the copy constructor
    LoadBalancer(const LoadBalancer &) = delete;
    LoadBalancer &operator=(const LoadBalancer &) = delete;

public:
    static LoadBalancer &getInstance() {
        static LoadBalancer lb;
        return lb;
    }
    void addServer(std::string url) {
        serverList.push_back(std::make_unique<Server>(url));
    }

    void removeServer(std::string url) {
        std::erase_if(serverList, [&](const std::unique_ptr<Server> &server) {
            return server->getUrl() == url;
        });
    }
    void listServers() {
        std::cout << "------------Registered Servers------------" << std::endl;

        for (const auto &server : serverList) {
            std::cout << server->getUrl() << std::endl;
        }

        std::cout << "------------------------------------------" << std::endl;
    }

    void setStrategy(std::unique_ptr<IRouteStrategy> strategy) {
        routeStrategy = std::move(strategy);
    }

    Server &routeRequest() {
        Server &selectedServer = routeStrategy->selectServer(serverList);

        // make http request using crp:
        return selectedServer;
    }
};

/**
 * @brief Forwards an incoming crow::request to a backend server and returns its response.
 *
 * This function acts as a reverse proxy, forwarding the original method, headers,
 * body, and query parameters.
 *
 * @param req The original incoming request from the client.
 * @param base_url The base URL of the backend server (e.g., "http://server-a.com").
 * @return A crow::response object to be sent back to the original client.
 */
crow::response forwardRequest(const crow::request& req, const std::string& base_url) {

    // 1. Construct the full target URL
    std::string target_url = base_url + req.url;
    
    // 2. Prepare Headers
    cpr::Header cpr_headers;
    for (const auto& header : req.headers) {
        cpr_headers[header.first] = header.second;
    }

    // 3. Prepare Body
    cpr::Body body{req.body};

    // 4. Send the request using the correct HTTP method
    cpr::Response r;
    r = cpr::Get(cpr::Url{target_url}, cpr_headers);
    
    // FIX 1: Get the numeric value of the enum for the switch
    // switch (static_cast<int>(req.method)) { 
    //     case static_cast<int>(crow::HTTPMethod::Get):
    //         r = cpr::Get(cpr::Url{target_url}, cpr_headers);
    //         break;
    //     case static_cast<int>(crow::HTTPMethod::Post):
    //         r = cpr::Post(cpr::Url{target_url}, body, cpr_headers);
    //         break;
    //     case static_cast<int>(crow::HTTPMethod::Put):
    //         r = cpr::Put(cpr::Url{target_url}, body, cpr_headers);
    //         break;
    //     case static_cast<int>(crow::HTTPMethod::Delete):
    //         r = cpr::Delete(cpr::Url{target_url}, body, cpr_headers);
    //         break;
    //     case static_cast<int>(crow::HTTPMethod::Patch):
    //         r = cpr::Patch(cpr::Url{target_url}, body, cpr_headers);
    //         break;
    //     case static_cast<int>(crow::HTTPMethod::Options):
    //         r = cpr::Options(cpr::Url{target_url}, cpr_headers);
    //         break;
    //     case static_cast<int>(crow::HTTPMethod::Head):
    //         r = cpr::Head(cpr::Url{target_url}, cpr_headers);
    //         break;
    //     default:
    //         return crow::response(400, "Bad Request: Unsupported HTTP method.");
    // }
    //
    // // 5. Build the crow::response from the cpr::response
    // crow::response res(r.status_code, r.text);
    //
    // // 6. Copy all headers from the backend response to the client response
    // for (const auto& header : r.header) {
    //     res.add_header(header.first, header.second);
    // }
    //
    // return res;
    //
    
    return crow::response(200);
}


int main() {
    AppConfig config = parseTomlFile("config.toml");

    crow::SimpleApp app;

    LoadBalancer &lb = LoadBalancer::getInstance();
    lb.addServer("www.google.com");
    lb.addServer("www.facebook.com");
    lb.addServer("www.x.com");
    lb.listServers();

    lb.setStrategy(StrategyFactory::getStrategy(config.strategy));

    // 3. 🌐 Define the main endpoint
    // This endpoint will handle all incoming requests to be balanced.
    CROW_ROUTE(app, "/").methods("GET"_method, "POST"_method)([]() {
        try {
            // Get the singleton instance inside the handler
            LoadBalancer &lb_instance = LoadBalancer::getInstance();

            // Delegate the work to the load balancer
            Server &chosen_server = lb_instance.routeRequest();

            // // Prepare a success response (JSON is a good choice)
            // crow::json::wvalue response_json;
            // response_json["status"] = "success";
            // response_json["routed_to"] = chosen_server.getUrl();


            std::cout << "Request routed to: " << chosen_server.getUrl() << std::endl;
            return forwardRequest(crow::request(), chosen_server.getUrl());
            // return crow::response(200, response_json);

        } catch (const std::exception &e) {
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
    std::cout << "🔥 Load Balancer server starting on port " << config.port
        << std::endl;
    app.port(config.port).multithreaded().run();
}
