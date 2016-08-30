

#include <stdio.h>
#include <tchar.h>
#include "LibmadDecoder.h"
// 分片段从文件播放MP3

int main()
{
//     WSADATA wsaData;
//     DWORD wVersionRequested = MAKEWORD(2, 2);
//     WSAStartup(wVersionRequested, &wsaData);

    CLibmadDecoder maddecoder;

    maddecoder.Play("Star Light Afar.mp3");
    //maddecoder.Play("xxxx.mp3");

    system("pause");
//     WSACleanup();
    return 0;
}