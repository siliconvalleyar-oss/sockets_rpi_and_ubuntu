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
