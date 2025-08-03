#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")

#define MAX_CLIENTS 10
#define BUF_SIZE 512

SOCKET clients[MAX_CLIENTS];
char clientNames[MAX_CLIENTS][32];
int clientCount = 0;
CRITICAL_SECTION cs;

unsigned __stdcall client_thread(void *param) {
    SOCKET clientSock = (SOCKET)param;
    int idx = -1;
    char buf[BUF_SIZE];
    int len;

    EnterCriticalSection(&cs);
    for (int i = 0; i < clientCount; i++) {
        if (clients[i] == clientSock) {
            idx = i;
            break;
        }
    }
    LeaveCriticalSection(&cs);

    while (1) {
        len = recv(clientSock, buf, BUF_SIZE - 1, 0);
        if (len <= 0) break;

        buf[len] = 0;

        char messageWithName[BUF_SIZE + 32];
        snprintf(messageWithName, sizeof(messageWithName), "%s: %s", clientNames[idx], buf);

        EnterCriticalSection(&cs);
        for (int i = 0; i < clientCount; i++) {
            if (clients[i] != clientSock) {
                send(clients[i], messageWithName, (int)strlen(messageWithName), 0);
            }
        }
        LeaveCriticalSection(&cs);
    }

    EnterCriticalSection(&cs);
    for (int i = 0; i < clientCount; i++) {
        if (clients[i] == clientSock) {
            for (int j = i; j < clientCount - 1; j++) {
                clients[j] = clients[j + 1];
                strncpy(clientNames[j], clientNames[j + 1], sizeof(clientNames[0]));
            }
            clientCount--;
            break;
        }
    }
    LeaveCriticalSection(&cs);

    closesocket(clientSock);
    printf("Client disconnected\n");
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET listenSock, clientSock;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    InitializeCriticalSection(&cs);

    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("WSAStartup failed\n");
        return 1;
    }

    listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSock == INVALID_SOCKET) {
        printf("socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(27015);

    if (bind(listenSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("bind failed: %d\n", WSAGetLastError());
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed: %d\n", WSAGetLastError());
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    printf("Server started. Waiting for clients...\n");

    while (1) {
        clientSock = accept(listenSock, (struct sockaddr*)&clientAddr, &clientAddrSize);
        if (clientSock == INVALID_SOCKET) {
            printf("accept failed: %d\n", WSAGetLastError());
            continue;
        }

        char nameBuf[32];
        int nameLen = recv(clientSock, nameBuf, sizeof(nameBuf) - 1, 0);
        if (nameLen <= 0) {
            closesocket(clientSock);
            continue;
        }
        nameBuf[nameLen] = 0;

        EnterCriticalSection(&cs);
        if (clientCount >= MAX_CLIENTS) {
            LeaveCriticalSection(&cs);
            printf("Max clients reached, rejecting\n");
            closesocket(clientSock);
            continue;
        }
        clients[clientCount] = clientSock;
        strncpy(clientNames[clientCount], nameBuf, sizeof(clientNames[0]) - 1);
        clientNames[clientCount][sizeof(clientNames[0]) - 1] = 0;
        clientCount++;
        LeaveCriticalSection(&cs);

        printf("Client connected: %s:%d Name: %s\n",
            inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), nameBuf);

        uintptr_t hThread = _beginthreadex(NULL, 0, client_thread, (void*)clientSock, 0, NULL);
        if (hThread == 0) {
            printf("Failed to create thread\n");
            closesocket(clientSock);
            EnterCriticalSection(&cs);
            clientCount--;
            LeaveCriticalSection(&cs);
        }
        CloseHandle((HANDLE)hThread);
    }

    DeleteCriticalSection(&cs);
    closesocket(listenSock);
    WSACleanup();
    return 0;
}