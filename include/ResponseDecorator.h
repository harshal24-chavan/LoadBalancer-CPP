#pragma once

#include "MetricsCalc.h"
#include "RequestHandler.h"
#include <memory>
#include <utility>

class ResponseDecorator : public IRequestHandler {
protected:
  std::unique_ptr<IRequestHandler> reqHandler;

public:
  ResponseDecorator(std::unique_ptr<IRequestHandler> req)
      : reqHandler(std::move(req)) {}

  ~ResponseDecorator() = default;
};

class MetricsDecorator : public ResponseDecorator {
private:
  MetricsCalc &metricsCalc;

public:
  MetricsDecorator(std::unique_ptr<IRequestHandler> handler)
      : ResponseDecorator(std::move(handler)),
        metricsCalc(MetricsCalc::getInstance()) {};

  crow::response handle(const crow::request &req) override;
};

class LoggingDecorator : public ResponseDecorator {
public:
  LoggingDecorator(std::unique_ptr<IRequestHandler> req)
      : ResponseDecorator(std::move(req)) {};

  crow::response handle(const crow::request &req) override;
};
