#include<atomic>
#include<string>

#include "Server.h"

Server::Server(std::string u) : url(u){
}

const std::string Server::getUrl() const{
    return url;
}

bool Server::checkHealth() const{
    // for now just return 1; // ok health
    return healthy.load();
}

void Server::markUnhealthy(){
    healthy = false;
}
void Server::markHealthy(){
    healthy = true;
}

void Server::incrementActiveConnection(){
    activeConnections++;
}
void Server::decrementActiveConnection(){
    activeConnections--;
}
int Server::getConnections() const {
    return activeConnections.load();
}


