#pragma once
#include <thread>
#include <memory>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <ms/string.h>
#include <ms/log/LogToFile.h>
#include <Ws2tcpip.h>
namespace ms{
    namespace socket{
        namespace tcp{
            class accept_server
            {
            public:
                accept_server(unsigned short port)
                    : port_(port)
                    , accept_func_(NULL)
                {;}
                ~accept_server(){StopAccept();}
                typedef int (*accept_call)(SOCKET socket);
                bool StartAccept(accept_call func)
                {
                    if (!thread_ptr_)
                    {
                        run_ = true;
                        accept_func_ = func;
                        thread_ptr_ = std::make_shared<std::thread>(std::bind(&accept_server::Accept, this));
                    }
                    return false;
                }
                void StopAccept()
                {
                    run_ = false;
                    if (thread_ptr_)
                    {
                        thread_ptr_->join();
                    }
                }

            private:
                SOCKET Accept()
                {
                    SOCKET listenSock = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
                    if (INVALID_SOCKET == listenSock)
                        return -1;
                    sockaddr_in serverAddr /*= { 0 }*/;
                    memset(&serverAddr, 0, sizeof(sockaddr_in));
                    if (ip_.empty())
                        serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); //any ip address
                    else
                    {
                        //serverAddr.sin_addr.s_addr = inet_addr(m_strIP.c_str()); //指定IP
                        InetPton(AF_INET, ip_.c_str(), &serverAddr.sin_addr);
                    }

                    int nRet = -1;
                    //设置允许本地地址重用
                    if (ip_.empty())
                    {
                        int iOptVal = 1;
                        int iOptLen = sizeof(int);

                        nRet = setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&iOptVal, iOptLen);
                        if (nRet == SOCKET_ERROR) {
                            AppErrorLogFormat(_T("getsockopt for SO_REUSEADDR failed with error: %u"), WSAGetLastError());
                        }
                        else
                            AppLogFormat(1, _T("SO_REUSEADDR Value: %ld"), iOptVal);
                    }

                    serverAddr.sin_family = AF_INET; // ipv4
                    serverAddr.sin_port = htons(port_);  //port 10000

                    nRet = ::bind(listenSock, (LPSOCKADDR)&serverAddr, sizeof(sockaddr));
                    if (SOCKET_ERROR == nRet)
                    {
                        AppErrorLogFormat(_T("bind failed with error %u"), WSAGetLastError());
                        ::closesocket(listenSock);
                        return -1;
                    }
                    nRet = ::listen(listenSock, 64); //listen connection 
                    if (SOCKET_ERROR == nRet)
                    {
                        closesocket(listenSock);
                        return -1;
                    }

                    //client addr
                    sockaddr_in clientAddr = { 0 };
                    int len = sizeof(sockaddr_in);
                    //等待连接
                    while (run_)
                    {
                        SOCKET clientSock = accept(listenSock, (sockaddr*)&clientAddr, &len);
                        if (clientSock == INVALID_SOCKET)
                        {
                            AppErrorLogFormat(_T("accetp() error! code: %d"), WSAGetLastError());
                            closesocket(listenSock);
                            return -1;
                        }

                        TCHAR addr[50] = { 0 };
                        const TCHAR* paddr = InetNtop(AF_INET, &clientAddr.sin_addr, addr, 50);
                        if (!paddr)
                        {
                            AppErrorLogFormat(_T("InetNtop() error! code: %d"), WSAGetLastError());
                        }
                        else
                            AppLogFormat(1, _T("client sock addr: %s, sock: %d"), addr, clientSock);

                        //AppErrorLogFormat(_T("client sock addr: %s", buf));

                        //
                        {
                            if (accept_func_)
                            {
                                accept_func_(clientSock);
                            }
                            //std::lock_guard<std::mutex> lg(*m_vtMutex);
                            //m_vtSock.push_back(std::move(clientSock));
                            clientSock = INVALID_SOCKET;
                        }
                    }
                    shutdown(listenSock, SD_BOTH);
                    closesocket(listenSock);
                    return 0;
                }

                unsigned short port_;
                ms::string ip_;
                std::shared_ptr<std::thread> thread_ptr_;
                bool run_;
                accept_call accept_func_;
            };

        }
    }
}