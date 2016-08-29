#pragma once
#include <thread>
#include <memory>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <ms/string.h>

namespace ms{
namespace socket{
namespace udp{

    class server
    {
    public:
        server(unsigned short port, const ms::string& ip = _T(""))
            :port_(port), ip_(ip), recv_func_(NULL), response_func_(NULL), error_func_(NULL)
        {}
        ~server(){ StopServer(); }
        typedef int(*recv_call)(const char * const buf, int buf_size);
        typedef int(*response_call)(const char *const buf, int buf_size);
        typedef int(*error_call)(DWORD err_code);

        bool StartServer(recv_call recv_func, response_call response_func, error_call error_func)
        {
            if (!thread_ptr_)
            {
                run_ = true;
                recv_func_ = recv_func;
                response_func_ = response_func;
                error_func_ = error_func;
                thread_ptr_ = std::make_shared<std::thread>(std::bind(&server::__run, this));
                return true;
            }
            return false;
        }
        void StopServer()
        {
            run_ = false;
            if (thread_ptr_)
            {
                thread_ptr_->join();
            }
        }
        
        int SendTo(const char *const  buf, int buf_size);

    private:
        void __run()
        {
            enum { max_buffer = 4096 };
            char buffer[max_buffer];
            SOCKET sock = INVALID_SOCKET;

            while (run_)
            {
                if (sock != INVALID_SOCKET)
                {
                    closesocket(sock);
                }
                sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
                if (INVALID_SOCKET == sock)
                {
                    break;
                }

                int nRet;
                SOCKADDR_IN addr;
                addr.sin_family = AF_INET;
                addr.sin_port = htons(port_);
                addr.sin_addr.s_addr = htonl(INADDR_ANY);

                DWORD budp = 1;		// 设置允许广播状态
                nRet = ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&budp, sizeof(budp));

                BOOL b = FALSE;		// 设置关闭后，立即释放资源
                nRet = ::setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (const char *)&b, sizeof(b));

                nRet = ::bind(sock, (struct sockaddr *)&addr, sizeof(SOCKADDR_IN));
                if (SOCKET_ERROR == nRet)
                {
                    ::closesocket(sock);
                    error_func_(SOCKET_ERROR);
                    break;
                }

                while (true)
                {
                    ////////////////////// 实际的音频采集 //////////////////////////
                    fd_set fdread;		FD_ZERO(&fdread);
                    FD_SET(sock, &fdread);
                    timeval tim;		tim.tv_sec = 5;	tim.tv_usec = 0;

                    ::select(0, &fdread, NULL, NULL, &tim);
                    if (!FD_ISSET(sock, &fdread))
                        break;
                    SOCKADDR from;
                    int len = sizeof(SOCKADDR);
                    nRet = ::recvfrom(sock, (char *)buffer, max_buffer, 0, &from, &len);
                    if (nRet <= 0)
                    {
                        DWORD dw = GetLastError();
                        if (error_func_)
                        {
                            error_func_(dw);
                        }
                        break;
                    }
                    if (recv_func_)
                    {
                        recv_func_(buffer, nRet);
                    }
                }
                shutdown(sock, SD_BOTH);
                closesocket(sock);
            }

        }
        ms::string ip_;
        unsigned short port_;
        std::shared_ptr<std::thread> thread_ptr_;
        recv_call recv_func_;
        response_call response_func_;
        error_call error_func_;
        bool run_;
    };


}
}
}