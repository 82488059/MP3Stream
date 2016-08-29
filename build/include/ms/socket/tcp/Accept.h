#pragma once

#include <ms/string.h>
#include <thread>
#include <WinSock2.h>

class Accept{

public:

    Accept(const unsigned short port, const ms::string& ip)
        : port_(port), ip_(ip)
    {
    }
    ~Accept(){}
    typedef bool (*accept_func)(SOCKET& sock);
    void StartAccept(accept_func func)
    {
        while (true)
        {
        }
    }

private:
    ms::string ip_;
    unsigned short port_;
    std::thread thread_;
};