# Makefile para o projeto Labirinto (OpenGL moderno)
CXX = g++
CXXFLAGS = -Wall -Iinclude -std=c++11
LDFLAGS = -lGLEW -lglfw -lGL -lpthread

SRC_DIR = src
BUILD_DIR = build

# Executáveis
HOST = host
CLIENTE = cliente

# Código fonte
HOST_SRC = $(SRC_DIR)/host.cpp
CLIENTE_SRC = $(SRC_DIR)/cliente.cpp

all: $(HOST) $(CLIENTE)

$(HOST): $(HOST_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(CLIENTE): $(CLIENTE_SRC)
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -f $(HOST) $(CLIENTE)

run-host: $(HOST)
	./$(HOST)

run-cliente: $(CLIENTE)
	./$(CLIENTE)

.PHONY: all clean run-host run-cliente
