
#include <WinSock2.h>

#include <stdio.h>
#include <tchar.h>
// #include "LibmadDecoder.h"
#include "WaveCode.h""
#include "AudioAnalyse.h"
#include "Wave.h"
// #pragma comment(lib, "libmad.lib")
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

// 从文件播放MP3
CWave g_wave;

int main()
{
    WSADATA wsaData;
    DWORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);
    WAVEFORMATEX g_waveformat;
    g_waveformat.cbSize = sizeof(WAVEFORMATEX);
    g_waveformat.wFormatTag = WAVE_FORMAT_PCM;
    g_waveformat.nChannels = 2;			// 声道数
    g_waveformat.nSamplesPerSec = 16000;		// 采样率
    g_waveformat.wBitsPerSample = 16;	// 采样精度;
    g_waveformat.nAvgBytesPerSec = g_waveformat.nChannels * g_waveformat.nSamplesPerSec * (g_waveformat.wBitsPerSample / 8);
    g_waveformat.nBlockAlign = g_waveformat.wBitsPerSample / 8 * g_waveformat.nChannels;

    g_wave.Start(&g_waveformat);

    FILE *fp = fopen("", "rb");
    enum {max_buffer = 40960};
    unsigned char buffer[max_buffer];
    int size = 0;
    as::services::CAudioDecode decode;

    SOCKET sock = INVALID_SOCKET;
    //     FILE * fp = fopen("xxxx.mp3", "wb+");
    while (1)
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
        addr.sin_port = htons(5000);
        addr.sin_addr.s_addr = htonl(INADDR_ANY);

        DWORD budp = 1;		// 设置允许广播状态
        nRet = ::setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (const char *)&budp, sizeof(budp));

        BOOL b = FALSE;		// 设置关闭后，立即释放资源
        nRet = ::setsockopt(sock, SOL_SOCKET, SO_DONTLINGER, (const char *)&b, sizeof(b));

        nRet = ::bind(sock, (struct sockaddr *)&addr, sizeof(SOCKADDR_IN));
        if (SOCKET_ERROR == nRet)
        {
            ::closesocket(sock);
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

            nRet = ::recvfrom(sock, (char *)buffer + size, 4096, 0, &from, &len);
            if (nRet <= 0)
            {
                DWORD dw = GetLastError();
                break;
            }
            size += nRet;
            short pcm[576];
            decode.Decode(buffer, 576, pcm, 576);
            g_wave.Play((char*)pcm, 576 * 4);
            memcpy(buffer, buffer + 576, size - 576);
            size -= 576;
        }
        shutdown(sock, SD_BOTH);
        closesocket(sock);
    }

    return 0;
}