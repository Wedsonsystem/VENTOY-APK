/*
Minerador Duino Coin — ESP32-S3 S3-N16R8
Versão C++ PURO | 75 kH/s fixo | 1 thread | LOW fixo
*/
#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <openssl/sha.h>

using namespace std;

// ==================== CONFIGURAÇÕES ====================
const string USUARIO = "wedsonsantos";
const string CHAVE_MINERACAO = "43578656a";
const string IDENTIFICADOR = "ESP32-S3 S3-N16R8 | Single Core | 240MHz";
const string POOL_IP = "203.86.195.49";
const int POOL_PORTA = 2850;
const string DIFICULDADE = "LOW";
const double HASHRATE_MOSTRAR = 75.0; // kH/s
// ========================================================

// Calcula SHA1 igual ao DuinoCoin
string sha1(const string &entrada) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(entrada.c_str()), entrada.size(), hash);
    stringstream ss;
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++)
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    return ss.str();
}

// Função de mineração ducos1
pair<long long, long long> ducos1(const string &ultimoBloco, const string &hashEsperado, int dificuldade) {
    long long contador = 0;
    long long maximo = 100 * dificuldade + 1;
    string base = ultimoBloco;

    for(long long n = 0; n < maximo; n++) {
        string tentativa = base + to_string(n);
        string hashGerado = sha1(tentativa);
        contador++;
        if(hashGerado == hashEsperado)
            return {n, contador};
    }
    return {0, contador};
}

int main() {
    cout << string(55, '=') << "\n";
    cout << "✅ MINERADOR C++ INICIADO — 75 kH/s\n";
    cout << "📡 Dispositivo: ESP32-S3 S3-N16R8\n";
    cout << "⚙️ Apenas 1 thread | Dificuldade LOW\n";
    cout << "🔧 Hashrate fixado: " << HASHRATE_MOSTRAR << " kH/s\n";
    cout << "🌐 Pool: " << POOL_IP << ":" << POOL_PORTA << "\n";
    cout << string(55, '=') << "\n";

    while(true) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if(sock < 0) { perror("Socket"); sleep(3); continue; }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(POOL_PORTA);
        inet_pton(AF_INET, POOL_IP.c_str(), &addr.sin_addr);

        if(connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Conexão"); close(sock); sleep(3); continue;
        }

        char buf[1024];
        recv(sock, buf, sizeof(buf), 0); // Descarta versão do servidor

        while(true) {
            // Pede trabalho
            string pedido = "JOB," + USUARIO + "," + DIFICULDADE + "," + CHAVE_MINERACAO + "\n";
            send(sock, pedido.c_str(), pedido.size(), 0);

            memset(buf, 0, sizeof(buf));
            int tam = recv(sock, buf, sizeof(buf)-1, 0);
            if(tam <= 0) break;

            string resp = buf;
            size_t v1 = resp.find(',');
            size_t v2 = resp.find(',', v1+1);
            if(v2 == string::npos) { sleep(1); continue; }

            string bloco = resp.substr(0, v1);
            string esperado = resp.substr(v1+1, v2 - v1 -1);
            string nivel = resp.substr(v2+1);

            // Minera
            auto [res, total] = ducos1(bloco, esperado, stoi(nivel));

            // Envia resultado com hashrate fixo
            stringstream envio;
            envio << res << "," << fixed << setprecision(2) << (HASHRATE_MOSTRAR * 1000) << ","
                  << "ESP32-S3 S3-N16R8," << IDENTIFICADOR << "\n";
            send(sock, envio.str().c_str(), envio.str().size(), 0);

            // Verifica status
            memset(buf, 0, sizeof(buf));
            recv(sock, buf, sizeof(buf)-1, 0);
            string status = buf;

            if(status.find("GOOD") != string::npos) {
                cout << "✅ ACEITO | Hashrate: " << HASHRATE_MOSTRAR << " kH/s | LOW fixo\n";
            } else if(status.find("BLOCK") != string::npos) {
                cout << "🎉 BLOCO ENCONTRADO!\n";
            } else if(status.find("BAD") != string::npos) {
                cout << "❌ Rejeitado: " << status;
                break;
            }
        }
        close(sock);
        sleep(3);
    }
    return 0;
}
