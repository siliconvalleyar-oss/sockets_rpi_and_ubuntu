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
