#include <iostream>
#include <fstream>
#include <string>
#include <csignal>

#include "protocol.h"
#include "socket_utils.h"

static bool g_verbose = true;

static void progress(size_t current, size_t total) {
    if (!g_verbose) return;
    std::cout << "\rProgreso: " << (current * 100 / total) << "%" << std::flush;
}

static bool sendFile(const std::string& filename,
                     const std::string& serverIP, int port,
                     std::chrono::milliseconds timeout) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: no se pudo abrir " << filename << std::endl;
        return false;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    try {
        Socket sock(AF_INET, SOCK_STREAM, 0);
        sock.setTimeout(timeout);

        auto addr = makeAddr(serverIP, port);
        sock.connect(addr);

        std::string meta = filename + "\n" + std::to_string(fileSize) + "\n";
        if (!sendAll(sock.fd(), meta.data(), meta.size())) {
            std::cerr << "Error: fallo al enviar metadatos" << std::endl;
            return false;
        }

        char buffer[BUFFER_SIZE];
        size_t totalSent = 0;
        while (totalSent < fileSize) {
            size_t toRead = std::min(fileSize - totalSent, (size_t)BUFFER_SIZE);
            file.read(buffer, static_cast<std::streamsize>(toRead));
            size_t bytesRead = static_cast<size_t>(file.gcount());
            if (bytesRead == 0) break;

            if (!sendAll(sock.fd(), buffer, bytesRead)) {
                std::cerr << "\nError: fallo al enviar datos" << std::endl;
                return false;
            }

            char ack;
            if (!recvAll(sock.fd(), &ack, ACK_SIZE)) {
                std::cerr << "\nError: no se recibio ACK" << std::endl;
                return false;
            }
            if (ack != ACK_OK) {
                std::cerr << "\nError: ACK erroneo" << std::endl;
                return false;
            }

            totalSent += bytesRead;
            progress(totalSent, fileSize);
        }

        char end = END_MARKER;
        sendAll(sock.fd(), &end, 1);

        if (g_verbose) std::cout << "\nArchivo enviado correctamente." << std::endl;
        return true;

    } catch (const SocketError& e) {
        std::cerr << "Error de socket: " << e.what() << std::endl;
        return false;
    }
}

static void usage(const char* prog) {
    std::cerr << "Uso: " << prog << " [opciones] <archivo> <IP> <puerto>\n"
              << "Opciones:\n"
              << "  -q        modo silencioso (sin progreso)\n"
              << "  -t <ms>   timeout en milisegundos (defecto: 5000)\n"
              << "Ejemplo:\n"
              << "  " << prog << " documento.pdf 192.168.1.100 5000\n"
              << "  " << prog << " -t 10000 imagen.jpg 10.0.0.5 5000\n";
}

int main(int argc, char* argv[]) {
    std::chrono::milliseconds timeout(5000);
    int optIdx = 1;

    while (optIdx < argc && argv[optIdx][0] == '-') {
        std::string opt(argv[optIdx]);
        if (opt == "-q") {
            g_verbose = false;
            optIdx++;
        } else if (opt == "-t" && optIdx + 1 < argc) {
            timeout = std::chrono::milliseconds(std::stoi(argv[optIdx + 1]));
            optIdx += 2;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    int remaining = argc - optIdx;
    if (remaining != 3) {
        usage(argv[0]);
        return 1;
    }

    std::string filename = argv[optIdx];
    std::string ip       = argv[optIdx + 1];
    int port             = std::stoi(argv[optIdx + 2]);

    return sendFile(filename, ip, port, timeout) ? 0 : 1;
}
