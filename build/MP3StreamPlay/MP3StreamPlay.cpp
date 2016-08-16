

#include <stdio.h>
#include <tchar.h>
#include "LibmadDecoder.h"
// 从文件播放MP3

int main()
{
    CLibmadDecoder maddecoder;

    maddecoder.Play("Star Light Afar.mp3");

    system("pause");
    return 0;
}