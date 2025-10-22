#include<atomic>
#include<string>

#include "Server.h"

Server::Server(std::string u) : url(u){
}

const std::string Server::getUrl() const{
  return url;
}

bool Server::checkHealth() const{
  return healthy.load();
}

void Server::setHealth(bool status){
  healthy.store(status);
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


