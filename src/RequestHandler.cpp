#include "RequestHandler.h"
#include "crow.h"
#include "loadbalancer.h"

#include <cpr/cpr.h>
#include <string>

cpr::Header ProxyHandler::convertHeaders(const crow::request &req) {
  cpr::Header headers;
  std::cout << "\nRequest Headers: " << std::endl;
  for (const auto &header : req.headers) {
    if (header.first != "Connection" && header.first != "Keep-Alive" &&
        header.first != "Transfer-Encoding" && header.first != "Host") {
      headers[header.first] = header.second;
      std::cout << header.first << ": " << header.second << std::endl;
    }
  }

  // Add X-Forwarded headers
  headers["X-Forwarded-For"] = req.remote_ip_address;
  headers["X-Forwarded-Proto"] = "http";
  headers["X-Real-IP"] = req.remote_ip_address;

  return headers;
}

crow::response ProxyHandler::convertResponse(const cpr::Response &cprRes) {
  std::cout << "CPR Response body: " << cprRes.text << std::endl;
  crow::response response(cprRes.status_code, cprRes.text);
  std::cout << "Crow Response Body: " << response.body << std::endl;

  // copy response headers
  for (const auto &header : cprRes.header) {
    response.add_header(header.first, header.second);
  }
  return response;
}

ProxyHandler::ProxyHandler(LoadBalancer &loadbalancer) : lb(loadbalancer) {}

crow::response ProxyHandler::handle(const crow::request &req) {
  for (int retry = 0; retry < 3; retry++) {
    Server &server = lb.getServer();

    try {
      std::string fullUrl = server.getUrl() + std::string(req.url);

      cpr::Header headers = convertHeaders(req);

      auto timeout = cpr::Timeout{30000};              // 30 second timeout
      auto connectTimeout = cpr::ConnectTimeout{5000}; // 5 seoond timeout

      cpr::Response cprRes;

      switch (req.method) {
      case crow::HTTPMethod::Get:
        cprRes = cpr::Get(cpr::Url(fullUrl), headers, timeout, connectTimeout);
        break;
      case crow::HTTPMethod::Post:
        cprRes = cpr::Post(cpr::Url(fullUrl), headers, cpr::Body{req.body},
                           timeout, connectTimeout);
        break;
      case crow::HTTPMethod::Put:
        cprRes = cpr::Put(cpr::Url{fullUrl}, headers, cpr::Body{req.body},
                          timeout, connectTimeout);
        break;
      case crow::HTTPMethod::Delete:
        cprRes =
            cpr::Delete(cpr::Url{fullUrl}, headers, timeout, connectTimeout);
        break;
      case crow::HTTPMethod::Patch:
        cprRes = cpr::Patch(cpr::Url{fullUrl}, headers, cpr::Body{req.body},
                            timeout, connectTimeout);
        break;
      case crow::HTTPMethod::Head:
        cprRes = cpr::Head(cpr::Url{fullUrl}, headers, timeout, connectTimeout);
        break;
      case crow::HTTPMethod::Options:
        cprRes =
            cpr::Options(cpr::Url{fullUrl}, headers, timeout, connectTimeout);
        break;
      default:
        return crow::response(405, "Method not allowed");
      }

      // Check if request was successful
      if (cprRes.error.message.empty()) {
        server.setHealthy(true);
        CROW_LOG_DEBUG << "Forwarded " << crow::method_name(req.method) << " "
                       << req.url << " to " << server.getUrl()
                       << " - Status: " << cprRes.status_code;

        return convertResponse(cprRes);
      } else {
        // Connection/network error
        server.setHealthy(false);
        CROW_LOG_ERROR << "Backend error (" << server.getUrl()
                       << "): " << cprRes.error.message;
        continue;
      }
    } catch (const std::exception &e) {
      server.setHealthy(false);
      CROW_LOG_ERROR << "Exception forwarding to " << server.getUrl() << ": "
                     << e.what();
      continue;
    }
  }

  return crow::response(502, "Bad Gateway - All backends failed");
}
