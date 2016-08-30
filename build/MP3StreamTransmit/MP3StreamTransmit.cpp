
#include "stdafx.h"
#include <stdio.h>
#include <tchar.h>
#include "LibmadDecoder.h"
#include <ms/socket/tcp/server.h>
#include <ms/string.h>
#include <ms/socket/udp/server.h>
#pragma comment(lib, "ws2_32.lib")
#include <list>
int udp_recv(const char * const buf, int size);
int udp_respoonse(const char * const buf, int size);
int udp_error(DWORD err_code);

int tcp_accept( SOCKET new_connect);

enum {recv_max = 40960};
char recv_buffer[recv_max];
volatile int recv_size = 0;
//std::list<SOCKET>  sock_list;
SOCKET sock = INVALID_SOCKET;

int main()
{
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    ms::socket::udp::server udp_server(5000);
    udp_server.StartServer(udp_recv, udp_respoonse, udp_error);


    ms::socket::tcp::accept_server tcp_server(8888);
    tcp_server.StartAccept(tcp_accept);

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
    if (INVALID_SOCKET == sock)
    {
        return 0;
    }
    int send = 0;
    do
    {
        int s = ::send(sock, buf, size, 0);
        if (s < 0)
        {
            ::closesocket(sock);
            sock = INVALID_SOCKET;
            return 0;
        }
        send += s;
    } while (send < size);

    //memcpy(recv_buffer + recv_size, buf, size);
    //recv_size += size;
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
int tcp_accept(SOCKET new_connect)
{
    sock = new_connect;

    return 0;
}
