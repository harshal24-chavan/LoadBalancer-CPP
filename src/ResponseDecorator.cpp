#include "ResponseDecorator.h"
#include "MetricsCalc.h"
#include "RequestHandler.h"
#include <chrono>

crow::response MetricsDecorator::handle(const crow::request &req) {

  // increasing the active connection count and total request count
  metricsCalc.incrementRequestCount();
  metricsCalc.incrementConnections();

  auto start = std::chrono::steady_clock::now();

  crow::response response = reqHandler->handle(req);

  auto end = std::chrono::steady_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  // add the response to total duration
  metricsCalc.setTotalResponseTimeMS(duration);

  // remove the active connection count
  metricsCalc.decreaseConnections();

  return response;
}

crow::response LoggingDecorator::handle(const crow::request &req) {
  std::cout << "Request: " << req.url << std::endl;
  crow::response response = reqHandler->handle(req);
  std::cout << "Response: " << response.body << std::endl;
  return response;
}
