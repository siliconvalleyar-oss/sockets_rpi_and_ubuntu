#!/bin/bash

# Crear directorios
mkdir -p bin obj include src

# ========================
# Archivo src/master.cpp
# ========================
cat > src/master.cpp << 'EOF'
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <string>

const int BUFFER_SIZE = 1024;
const int ACK_SIZE = 1;

bool sendFile(const std::string& filename, const std::string& serverIP, int port) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir " << filename << std::endl;
        return false;
    }
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error al crear socket" << std::endl;
        return false;
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "IP inválida" << std::endl;
        close(sock);
        return false;
    }
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error de conexión" << std::endl;
        close(sock);
        return false;
    }

    // Enviar metadatos
    std::string metadata = filename + "\n" + std::to_string(fileSize) + "\n";
    send(sock, metadata.c_str(), metadata.size(), 0);

    char buffer[BUFFER_SIZE];
    size_t totalSent = 0;
    while (totalSent < fileSize) {
        size_t toRead = std::min(fileSize - totalSent, (size_t)BUFFER_SIZE);
        file.read(buffer, toRead);
        size_t bytesRead = file.gcount();
        if (bytesRead == 0) break;
        send(sock, buffer, bytesRead, 0);
        char ack;
        recv(sock, &ack, ACK_SIZE, 0);
        if (ack != 'K') {
            std::cerr << "ACK erróneo" << std::endl;
            close(sock);
            return false;
        }
        totalSent += bytesRead;
        std::cout << "Progreso: " << (totalSent * 100 / fileSize) << "%\r" << std::flush;
    }
    std::cout << std::endl;
    char end = 'E';
    send(sock, &end, 1, 0);
    close(sock);
    std::cout << "Archivo enviado correctamente." << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Uso: " << argv[0] << " <archivo> <IP_esclavo> <puerto>" << std::endl;
        return 1;
    }
    sendFile(argv[1], argv[2], std::stoi(argv[3]));
    return 0;
}
EOF

# ========================
# Archivo src/slave.cpp
# ========================
cat > src/slave.cpp << 'EOF'
#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>   // Para inet_ntoa
#include <unistd.h>
#include <string>

const int BUFFER_SIZE = 1024;
const int ACK_SIZE = 1;

bool receiveFile(int clientSock) {
    char metadata[256];
    int bytes = recv(clientSock, metadata, sizeof(metadata)-1, 0);
    if (bytes <= 0) return false;
    metadata[bytes] = '\0';
    std::string metadataStr(metadata);
    size_t newlinePos = metadataStr.find('\n');
    if (newlinePos == std::string::npos) return false;
    std::string filename = metadataStr.substr(0, newlinePos);
    std::string rest = metadataStr.substr(newlinePos+1);
    newlinePos = rest.find('\n');
    if (newlinePos == std::string::npos) return false;
    size_t fileSize = std::stoull(rest.substr(0, newlinePos));

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error creando " << filename << std::endl;
        return false;
    }

    char buffer[BUFFER_SIZE];
    size_t totalReceived = 0;
    while (totalReceived < fileSize) {
        size_t toRead = std::min(fileSize - totalReceived, (size_t)BUFFER_SIZE);
        int recvBytes = recv(clientSock, buffer, toRead, 0);
        if (recvBytes <= 0) break;
        outFile.write(buffer, recvBytes);
        totalReceived += recvBytes;
        char ack = 'K';
        send(clientSock, &ack, ACK_SIZE, 0);
        std::cout << "Recibido: " << (totalReceived * 100 / fileSize) << "%\r" << std::flush;
    }
    std::cout << std::endl;
    char end;
    recv(clientSock, &end, 1, 0);
    outFile.close();
    std::cout << "Archivo " << filename << " recibido (" << totalReceived << " bytes)" << std::endl;
    return true;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <puerto>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[1]);

    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "Error al crear socket" << std::endl;
        return 1;
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    if (bind(serverSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error en bind" << std::endl;
        close(serverSock);
        return 1;
    }
    listen(serverSock, 1);
    std::cout << "Esperando conexión en puerto " << port << "..." << std::endl;
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    int clientSock = accept(serverSock, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientSock < 0) {
        std::cerr << "Error en accept" << std::endl;
        close(serverSock);
        return 1;
    }
    std::cout << "Conectado: " << inet_ntoa(clientAddr.sin_addr) << std::endl;
    bool ok = receiveFile(clientSock);
    close(clientSock);
    close(serverSock);
    return ok ? 0 : 1;
}
EOF

# ========================
# Makefile
# ========================
cat > Makefile << 'EOF'
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -Iinclude
OBJ_DIR = obj
BIN_DIR = bin

_OBJ_MASTER = master.o
_OBJ_SLAVE = slave.o
OBJ_MASTER = $(addprefix $(OBJ_DIR)/, $(_OBJ_MASTER))
OBJ_SLAVE = $(addprefix $(OBJ_DIR)/, $(_OBJ_SLAVE))

all: master slave

master: $(OBJ_MASTER) | $(BIN_DIR)
	$(CXX) $^ -o $(BIN_DIR)/$@

slave: $(OBJ_SLAVE) | $(BIN_DIR)
	$(CXX) $^ -o $(BIN_DIR)/$@

$(OBJ_DIR)/%.o: src/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -f $(OBJ_DIR)/*.o $(BIN_DIR)/master $(BIN_DIR)/slave

.PHONY: all master slave clean
EOF

echo "Estructura creada: bin/, obj/, include/, src/"
echo "Compila con:"
echo "  make master   -> genera bin/master"
echo "  make slave    -> genera bin/slave"
echo "  make all      -> genera ambos"
echo "Ejemplo de uso:"
echo "  En Raspberry: ./bin/slave 5000"
echo "  En PC:        ./bin/master archivo.txt 192.168.1.10 5000"


