#include "../include/Network.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int Network::createSocket() {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    cerr << "Erro ao criar socket: " << strerror(errno) << endl;
  }
  return sock;
}

bool Network::bindAndListen(int socket, int port) {
  // Permitir reutilização da porta
  int opt = 1;
  setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  if (::bind(socket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) <
      0) {
    cerr << "Erro no bind: " << strerror(errno) << endl;
    return false;
  }

  if (listen(socket, 1) < 0) {
    cerr << "Erro no listen: " << strerror(errno) << endl;
    return false;
  }

  return true;
}

int Network::acceptConnection(int serverSocket) {
  int clientSocket = accept(serverSocket, nullptr, nullptr);
  if (clientSocket < 0) {
    cerr << "Erro ao aceitar conexão: " << strerror(errno) << endl;
  }
  return clientSocket;
}

bool Network::connectToServer(int socket, const string &ip, int port) {
  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);

  if (inet_pton(AF_INET, ip.c_str(), &serverAddress.sin_addr) <= 0) {
    cerr << "Endereço inválido: " << ip << endl;
    return false;
  }

  if (connect(socket, (struct sockaddr *)&serverAddress,
              sizeof(serverAddress)) < 0) {
    cerr << "Erro ao conectar ao servidor: " << strerror(errno) << endl;
    return false;
  }

  return true;
}

bool Network::sendData(int socket, const void *data, size_t size) {
  ssize_t bytesSent = send(socket, data, size, 0);
  if (bytesSent < 0) {
    cerr << "Erro ao enviar dados: " << strerror(errno) << endl;
    return false;
  }
  return true;
}

ssize_t Network::receiveData(int socket, void *buffer, size_t size) {
  return recv(socket, buffer, size, 0);
}

void Network::closeSocket(int &socket) {
  if (socket >= 0) {
    close(socket);
    socket = -1;
  }
}