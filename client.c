#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

SOCKET sock;

unsigned __stdcall recv_thread(void *param) {
    char buf[512];
    int len;
    while (1) {
        len = recv(sock, buf, sizeof(buf) - 1, 0);
        if (len <= 0) {
            printf("\nConnection closed or error\n");
            ExitThread(0);
        }
        buf[len] = 0;
        printf("\n[Server]: %s\n> ", buf);
        fflush(stdout);
    }
}

int main() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(27015);
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("connect failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    char name[32];
    printf("Enter your name: ");
    if (!fgets(name, sizeof(name), stdin)) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '\n') name[len - 1] = 0;

    send(sock, name, (int)strlen(name), 0);

    uintptr_t hThread = _beginthreadex(NULL, 0, recv_thread, NULL, 0, NULL);
    if (hThread == 0) {
        printf("Failed to create thread\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Connected! Type messages, 'exit' to quit.\n");

    char msg[512];
    while (1) {
        printf("> ");
        if (!fgets(msg, sizeof(msg), stdin)) break;
        len = strlen(msg);
        if (len > 0 && msg[len - 1] == '\n') msg[len - 1] = 0;

        if (strcmp(msg, "exit") == 0) break;

        int sent = send(sock, msg, (int)strlen(msg), 0);
        if (sent == SOCKET_ERROR) {
            printf("send failed: %d\n", WSAGetLastError());
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}