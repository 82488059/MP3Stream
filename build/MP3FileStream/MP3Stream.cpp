// MP3Stream.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
    FILE* fp = NULL;
    errno_t err = fopen_s(&fp, "Star Light Afar.mp3", "rb");


	return 0;
}


