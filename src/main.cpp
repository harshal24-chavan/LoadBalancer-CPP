#include "crow.h"

int main(){
    crow::SimpleApp loadBalancer;

    CROW_ROUTE(loadBalancer, "/")([](){
        return "hello world";
    });

    loadBalancer.port(18080).multithreaded().run();

    return 0;
}
