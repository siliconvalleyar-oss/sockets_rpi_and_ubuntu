#include <iostream>
#include <fstream>
#include <string>
#include <csignal>
#include <cstring>
#include <sys/wait.h>

#include "protocol.h"
#include "socket_utils.h"

static bool g_running = true;
static std::string g_outDir = ".";

static void handleSigint(int) {
    std::cerr << "\nCerrando servidor..." << std::endl;
    g_running = false;
}

static void handleSigchld(int) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

static bool receiveFile(int clientSock, const std::string& peerIP) {
    char metaBuf[512];
    int metaLen;
    if (!recvSome(clientSock, metaBuf, sizeof(metaBuf) - 1, metaLen)) {
        std::cerr << "Error: no se recibieron metadatos de " << peerIP << std::endl;
        return false;
    }
    metaBuf[metaLen] = '\0';

    std::string metaStr(metaBuf);
    size_t nl1 = metaStr.find('\n');
    if (nl1 == std::string::npos) return false;
    std::string filename = metaStr.substr(0, nl1);

    size_t nl2 = metaStr.find('\n', nl1 + 1);
    if (nl2 == std::string::npos) return false;
    size_t fileSize = std::stoull(metaStr.substr(nl1 + 1, nl2 - nl1 - 1));

    std::string outPath = g_outDir + "/" + filename;
    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Error creando " << outPath << std::endl;
        return false;
    }

    char buffer[BUFFER_SIZE];
    size_t totalReceived = 0;
    while (totalReceived < fileSize) {
        size_t toRead = std::min(fileSize - totalReceived, (size_t)BUFFER_SIZE);
        int recvBytes;
        if (!recvSome(clientSock, buffer, toRead, recvBytes)) {
            std::cerr << "\nError: conexion perdida recibiendo datos" << std::endl;
            return false;
        }
        outFile.write(buffer, recvBytes);
        totalReceived += recvBytes;

        char ack = ACK_OK;
        if (!sendAll(clientSock, &ack, ACK_SIZE)) {
            std::cerr << "\nError: fallo al enviar ACK" << std::endl;
            return false;
        }

        std::cout << "\r[" << peerIP << "] Recibido: "
                  << (totalReceived * 100 / fileSize) << "%" << std::flush;
    }

    char end;
    recvAll(clientSock, &end, 1);

    outFile.close();
    std::cout << "\n[" << peerIP << "] Archivo " << filename
              << " recibido (" << totalReceived << " bytes)" << std::endl;
    return true;
}

static void handleClient(Socket client, struct sockaddr_in peer) {
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &peer.sin_addr, ipStr, sizeof(ipStr));
    std::string peerIP(ipStr);

    std::cout << "Conectado: " << peerIP << ":" << ntohs(peer.sin_port) << std::endl;
    receiveFile(client.fd(), peerIP);
}

static void usage(const char* prog) {
    std::cerr << "Uso: " << prog << " [opciones] <puerto>\n"
              << "Opciones:\n"
              << "  -o <dir>  directorio de salida (defecto: .)\n"
              << "  -d        modo daemon (background)\n"
              << "Ejemplo:\n"
              << "  " << prog << " 5000\n"
              << "  " << prog << " -o ~/recibidos 5000\n";
}

int main(int argc, char* argv[]) {
    bool runDaemon = false;
    int optIdx = 1;

    while (optIdx < argc && argv[optIdx][0] == '-') {
        std::string opt(argv[optIdx]);
        if (opt == "-o" && optIdx + 1 < argc) {
            g_outDir = argv[optIdx + 1];
            optIdx += 2;
        } else if (opt == "-d") {
            runDaemon = true;
            optIdx++;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (optIdx >= argc) {
        usage(argv[0]);
        return 1;
    }
    int port = std::stoi(argv[optIdx]);

    signal(SIGINT, handleSigint);
    signal(SIGCHLD, handleSigchld);

    if (runDaemon && ::daemon(0, 0) < 0) {
        std::cerr << "Error: no se pudo hacer daemon" << std::endl;
        return 1;
    }

    try {
        Socket server = makeServerSocket(port);
        std::cout << "Servidor escuchando en puerto " << port
                  << " (directorio: " << g_outDir << ")" << std::endl;

        while (g_running) {
            try {
                struct sockaddr_in peer;
                Socket client = server.accept(&peer);
                pid_t pid = fork();
                if (pid < 0) {
                    std::cerr << "Error: fork fallo" << std::endl;
                    continue;
                }
                if (pid == 0) {
                    server.close();
                    signal(SIGINT, SIG_DFL);
                    handleClient(std::move(client), peer);
                    _exit(0);
                }
            } catch (const SocketError& e) {
                if (g_running)
                    std::cerr << "Error en accept: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
