

#include <stdio.h>
#include <tchar.h>
#include "LibmadDecoder.h"
#include <ms/socket/tcp/Accept.h>
#include <ms/string.h>
#include <ms/socket/udp/server.h>
#pragma comment(lib, "ws2_32.lib")
int udp_recv(const char * const buf, int size);
int udp_respoonse(const char * const buf, int size);
int udp_error(DWORD err_code);
enum {recv_max = 40960};
char recv_buffer[recv_max];
volatile int recv_size = 0;

int main()
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    ms::socket::udp::server udp_server(5000);
    udp_server.StartUdp(udp_recv, udp_respoonse, udp_error);

    system("pause");
    WSACleanup();
    return 0;
}

int udp_recv(const char * const buf, int size)
{
    if (recv_size + size > recv_max)
    {
        return 0;
    }
    memcpy(recv_buffer + recv_size, buf, size);
    recv_size += size;
    return 0;
}
int udp_respoonse(const char * const buf, int size)
{
    return 0;
}
int udp_error(DWORD err_code)
{
    return 0;
}
