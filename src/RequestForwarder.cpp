#include "crow.h"
#include<cpr/cpr.h>

#include<iostream>
#include<string>

#include "loadbalancer.h"
#include "Server.h"
#include "RequestForwarder.h"

cpr::Header RequestForwarder::convertHeaders(const crow::request& req){
  cpr::Header headers;
  std::cout<<"\nRequest Headers: "<<std::endl;
  for(const auto& header: req.headers){
    if(header.first != "Connection" && 
      header.first != "Keep-Alive" &&
      header.first != "Transfer-Encoding" &&
      header.first != "Host"){
      headers[header.first] = header.second;
      std::cout<<header.first<<": "<<header.second<<std::endl;
    }
  }

  // Add X-Forwarded headers
  headers["X-Forwarded-For"] = req.remote_ip_address;
  headers["X-Forwarded-Proto"] = "http";
  headers["X-Real-IP"] = req.remote_ip_address;

  return headers;
}

crow::response RequestForwarder::convertResponse(const cpr::Response& cprRes){
  std::cout<<"CPR Response body: "<<cprRes.text<<std::endl;
  crow::response response(cprRes.status_code, cprRes.text);
  std::cout<<"Crow Response Body: "<<response.body<<std::endl;

  // copy response headers
  for(const auto& header: cprRes.header){
    response.add_header(header.first, header.second);
  }
  return response;
}

RequestForwarder::RequestForwarder(LoadBalancer& loadbalancer) : lb(loadbalancer){}

crow::response RequestForwarder::forward(const crow::request& req, int maxRetries){
  int retries = 0;
  while(retries < maxRetries){
    Server& backend = lb.getServer();
    try{
      // std::string fullUrl = backend.getUrl() + std::string(req.url);
      std::string fullUrl = backend.getUrl() ;
      std::cout<<"Forwarding request to: "<<fullUrl<<std::endl;

      cpr::Header headers = convertHeaders(req);

      auto timeout = cpr::Timeout{30000}; // 30 second timeout
      auto connectTimeout = cpr::ConnectTimeout{5000}; // 5 seoond timeout

      cpr::Response cprRes;

      switch(req.method){
        case crow::HTTPMethod::Get:
          cprRes = cpr::Get(
            cpr::Url(fullUrl),
            headers,
            timeout,
            connectTimeout
          );
          break;
        case crow::HTTPMethod::Post:
          cprRes = cpr::Post(
            cpr::Url(fullUrl),
            headers,
            cpr::Body{req.body},
            timeout,
            connectTimeout
          );
          break;
        case crow::HTTPMethod::Put:
          cprRes = cpr::Put(
            cpr::Url{fullUrl},
            headers,
            cpr::Body{req.body},
            timeout,
            connectTimeout
          );
          break;                
        case crow::HTTPMethod::Delete:
          cprRes = cpr::Delete(
            cpr::Url{fullUrl},
            headers,
            timeout,
            connectTimeout
          );
          break;
        case crow::HTTPMethod::Patch:
          cprRes = cpr::Patch(
            cpr::Url{fullUrl},
            headers,
            cpr::Body{req.body},
            timeout,
            connectTimeout
          );
          break;
        case crow::HTTPMethod::Head:
          cprRes = cpr::Head(
            cpr::Url{fullUrl},
            headers,
            timeout,
            connectTimeout
          );
          break;

        case crow::HTTPMethod::Options:
          cprRes = cpr::Options(
            cpr::Url{fullUrl},
            headers,
            timeout,
            connectTimeout
          );
          break;

        default:
          return crow::response(405, "Method not allowed");
      }

      // Check if request was successful
      if (cprRes.error.message.empty()) {
        backend.setHealth(true);
        CROW_LOG_DEBUG << "Forwarded " << crow::method_name(req.method) 
          << " " << req.url << " to " << backend.getUrl()
          << " - Status: " << cprRes.status_code;
        return convertResponse(cprRes);
      } else {
        // Connection/network error
        backend.setHealth(false);
        CROW_LOG_ERROR << "Backend error (" << backend.getUrl() << "): " 
          << cprRes.error.message;
        retries++;
        continue;
      }
    }
    catch (const std::exception& e) {
      backend.setHealth(false);
      CROW_LOG_ERROR << "Exception forwarding to " << backend.getUrl()
        << ": " << e.what();
      retries++;
      continue;
    }
  }
  return crow::response(502, "Bad Gateway - All backends failed");
}
