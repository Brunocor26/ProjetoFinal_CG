#ifndef NETWORK_H
#define NETWORK_H

#include <netinet/in.h>
#include <string>

class Network {
public:
  static int createSocket();
  static bool bindAndListen(int socket, int port);
  static int acceptConnection(int serverSocket);
  static bool connectToServer(int socket, const std::string &ip, int port);
  static bool sendData(int socket, const void *data, size_t size);
  static ssize_t receiveData(int socket, void *buffer, size_t size);
  static void closeSocket(int &socket);
};

#endif
