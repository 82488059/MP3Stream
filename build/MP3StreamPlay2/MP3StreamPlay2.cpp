

#include <stdio.h>
#include <tchar.h>
#include "LibmadDecoder.h"
// ���ļ�����MP3

int main()
{
    WSADATA wsaData;
    DWORD wVersionRequested = MAKEWORD(2, 2);
    WSAStartup(wVersionRequested, &wsaData);

    CLibmadDecoder maddecoder;

    maddecoder.Play("udp");

    system("pause");
    WSACleanup();
    return 0;
}