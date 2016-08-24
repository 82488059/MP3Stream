// LibmadDecoder.cpp: implementation of the CLibmadDecoder class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
#include "LibmadDecoder.h"
// #include <tchar.h>
//#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")

void WINAPI DecodeThread(CLibmadDecoder* pLibmadDecoder)
{
	pLibmadDecoder->StartDecode();
}

void WINAPI RecvStreamThread(CLibmadDecoder *pLibmadDecoder)
{
    pLibmadDecoder->StartRecvStream();
}

void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	switch(uMsg)
	{
	case WOM_DONE: 
		{
			WAVEHDR *header = (WAVEHDR*)dwParam1;
			CLibmadDecoder::PCM_BLOCK* pPCM = (CLibmadDecoder::PCM_BLOCK*)header->dwUser;
			SetEvent(pPCM->hPlayEvent);
		}
		break;

	case WOM_OPEN:
	case WOM_CLOSE:
		break;
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLibmadDecoder::CLibmadDecoder()
{
    InitializeCriticalSection(&m_cs);
    fp_ = NULL;
	Init();
}

CLibmadDecoder::~CLibmadDecoder()
{
	Release();
    DeleteCriticalSection(&m_cs);
}

void CLibmadDecoder::Init()
{
	m_hWnd = NULL;
	m_hWaveOut = NULL;
	m_hThread = NULL;
	m_hEventPause = NULL;
	m_pFileBuffer = NULL;
	m_pFileBuffer = NULL;
	m_nBufIndex = 0;
	m_nFilebufferLen = 0;
	m_bOpened = FALSE;
	m_nPlayingStatus = eStoped;

	if(!m_outBuffer.data)
	{
		m_outBuffer.data = (unsigned char*)malloc(MAX_OUTPUTBUFFER);
	}
	m_hEventPause = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (fp_)
    {
        fclose(fp_);
        fp_ = NULL;
    }
    read_ = 0;
    thread_ = NULL; 
    udp_ = false;
    recv_ = true;
    size_ = 0;
}

void CLibmadDecoder::Release()
{
	// Stop playing
	Stop();

	// And waiting for ending
	Sleep(50);

	// Release the decoder
	mad_decoder_finish(&m_Decoder);

	// Release buffers
	if(m_outBuffer.data)
	{
		free((void*)m_outBuffer.data);
		m_outBuffer.data = NULL;
	}
	if(m_pFileBuffer)
	{
		free((void*)m_pFileBuffer);
		m_pFileBuffer = NULL;
	}
    if (fp_)
    {
        fclose(fp_);
        fp_ = NULL;
    }
    recv_ = false;

}

enum mad_flow CLibmadDecoder::input_func(void *data, struct mad_stream *stream)
{
	CLibmadDecoder* pThis = (CLibmadDecoder*)data;

	if(!pThis->m_nFilebufferLen)
	{
        pThis->m_nFilebufferLen = pThis->LoadFile2Memory(pThis->m_szFileName, (char**)&pThis->m_pFileBuffer);
        if (!pThis->m_nFilebufferLen)
        {
            pThis->m_nPlayingStatus = eStoped;
            return MAD_FLOW_STOP;
        }
        else
        {
            mad_stream_buffer(stream, pThis->m_pFileBuffer, pThis->m_nFilebufferLen);
            pThis->m_nFilebufferLen = 0;
        }
	}
	else
	{
		mad_stream_buffer(stream, pThis->m_pFileBuffer, pThis->m_nFilebufferLen);
		pThis->m_nFilebufferLen = 0;
	}
	
	return MAD_FLOW_CONTINUE;
}

enum mad_flow CLibmadDecoder::header_func(void *data, struct mad_header const*)
{
	CLibmadDecoder* pThis = (CLibmadDecoder*)data;
	return MAD_FLOW_CONTINUE;
}

enum mad_flow CLibmadDecoder::filter_func(void *data, struct mad_stream const*stream, struct mad_frame* frame)
{
	CLibmadDecoder* pThis = (CLibmadDecoder*)data;
	return MAD_FLOW_CONTINUE;
}

enum mad_flow CLibmadDecoder::output_func(void *data, struct mad_header const *header, struct mad_pcm *pcm)
{
	CLibmadDecoder* pThis = (CLibmadDecoder*)data;

	mad_fixed_t const *left_ch = pcm->samples[0];
	mad_fixed_t const *right_ch = pcm->samples[1];
	static unsigned int sample_count = 0;
	signed int sample0, sample1;
	int nsamples = pcm->length;
	float current_time = 0;

	// Calculate the current time
	sample_count += nsamples;
	current_time = (float)sample_count / pcm->samplerate;

	PCM_BLOCK *pPcmBlock = &pThis->m_PcmBlock[pThis->m_nBufIndex];

	// Waiting for resume
	if(pThis->m_nPlayingStatus == ePaused)
	{
		while(WaitForSingleObject(pThis->m_hEventPause, INFINITE) != WAIT_OBJECT_0)
		{
			Sleep(5);
		}
	}
	else if(pThis->m_nPlayingStatus == eStoped)
	{
		return MAD_FLOW_STOP;
	}

	if(pPcmBlock->bIsPlaying)
	{
		// The buffer is playing
		if(WaitForSingleObject(pPcmBlock->hPlayEvent, INFINITE) != WAIT_TIMEOUT)
		{
			waveOutUnprepareHeader(pThis->m_hWaveOut, &pPcmBlock->pWaveHdr, sizeof(pPcmBlock->pWaveHdr));
			pPcmBlock->bIsPlaying = FALSE;
			pPcmBlock->release();
		}
	}

	// Case of mono
	if(pcm->channels == 1)
	{
		right_ch = NULL;
	}

	// Combine decoded data to 16 bits PCM data
	pThis->m_outBuffer.length = pThis->CombinePCM16(&pThis->m_outBuffer.data[0], nsamples, left_ch, right_ch);

	// Prepare header info
	char* pData = (char*)malloc(pThis->m_outBuffer.length);
	memcpy(pData, pThis->m_outBuffer.data, pThis->m_outBuffer.length);
	pPcmBlock->pWaveHdr.lpData = pData;
	pPcmBlock->pWaveHdr.dwBufferLength = pThis->m_outBuffer.length;
	pPcmBlock->pWaveHdr.dwUser = (DWORD)pPcmBlock;
	pPcmBlock->pWaveHdr.dwFlags = 0;
	pPcmBlock->pWaveHdr.dwLoops = 0;

	// Set wave format and open a default waveform-audio device
	if(!pThis->m_bOpened)
	{
		pThis->SetWaveFormat(pcm->channels, pcm->samplerate, 16);
		if(!pThis->OpenDevice())
		{
			return MAD_FLOW_STOP;
		}
	}

	// Write PCM data to waveform-audio device
	pThis->WriteDevice(pPcmBlock);

	// Set playing flag and switch the double buffer
	pPcmBlock->bIsPlaying = TRUE;
	pThis->m_nBufIndex = (pThis->m_nBufIndex + 1) % 2;

	return MAD_FLOW_CONTINUE;
}

enum mad_flow CLibmadDecoder::error_func(void *data, struct mad_stream *stream, struct mad_frame *frame)
{
	CLibmadDecoder* pThis = (CLibmadDecoder*)data;
	return MAD_FLOW_CONTINUE;
}

int CLibmadDecoder::StartDecode()
{
	int nResult = -1;

	m_nFilebufferLen = LoadFile2Memory(m_szFileName, (char**)&m_pFileBuffer);

	mad_decoder_options(&m_Decoder, MAD_OPTION_IGNORECRC);
	
	// Configure input, output, and error functions
	mad_decoder_init(&m_Decoder, 
		this,
		input_func,	 // Input callback
		0,           // Header callback
		filter_func, // Filter callback
		output_func, // Output callback
		error_func,  // Error callback
		0			 // Message callback
	);

	// Start decoding
	nResult = mad_decoder_run(&m_Decoder, MAD_DECODER_MODE_SYNC);

	return nResult;
}


//
// The following utility routine performs simple rounding, clipping, and
// scaling of MAD's high-resolution samples down to specified bits. It does not
// perform any dithering or noise shaping, which would be recommended to
// obtain any exceptional audio quality. It is therefore not recommended to
// use this routine if high-quality output is desired.
//
signed int CLibmadDecoder::Scale(unsigned int bits, mad_fixed_t sample)
{
	// Round
	sample += (1L << (MAD_F_FRACBITS - bits));
	
	// Clip
	if(sample >= MAD_F_ONE)
	{
		sample = MAD_F_ONE - 1;
	}
	else if(sample < -MAD_F_ONE)
	{
		sample = -MAD_F_ONE;
	}
	
	// Quantize
	return sample >> (MAD_F_FRACBITS + 1 - bits);
}

unsigned int CLibmadDecoder::CombinePCM16(unsigned char *data, 
	unsigned int nsamples, mad_fixed_t const *left, mad_fixed_t const *right)
{
	unsigned int len = nsamples;
	register signed int sample0, sample1;
	
	// Case of stereo
	if(right)
	{
		while(len--) 
		{
			sample0 = Scale(16, *left++);
			sample1 = Scale(16, *right++);
			data[0] = sample0 >> 0;
			data[1] = sample0 >> 8;
			data[2] = sample1 >> 0;
			data[3] = sample1 >> 8;
			data += 4;
		}
		return nsamples * 2 * 2;
	}
	else // Case of mono
	{
		while(len--)
		{
			sample0 = Scale(16, *left++);
			data[0] = sample0 >> 0;
			data[1] = sample0 >> 8;
			data += 2;
		}
		return nsamples * 2;
	}
}

unsigned int CLibmadDecoder::CombinePCM24(unsigned char *data, 
	unsigned int nsamples, mad_fixed_t const *left, mad_fixed_t const *right)
{
	unsigned int len = nsamples;
	register signed long sample0, sample1;

	// Case of stereo
	if(right)
	{
		while(len--) 
		{
			sample0 = Scale(24, *left++);
			sample1 = Scale(24, *right++);
			data[0] = sample0 >>  0;
			data[1] = sample0 >>  8;
			data[2] = sample0 >> 16;
			data[3] = sample1 >>  0;
			data[4] = sample1 >>  8;
			data[5] = sample1 >> 16;
			data += 6;
		}
		return nsamples * 3 * 2;
	}
	else // Case of mono
	{
		while(len--) 
		{
			sample0 = Scale(24, *left++);	
			data[0] = sample0 >>  0;
			data[1] = sample0 >>  8;
			data[2] = sample0 >> 16;
			data += 3;
		}
		return nsamples * 3;
	}
}

BOOL CLibmadDecoder::Play(char* pszFileName, HWND hWnd/* = NULL*/)
{
	DWORD hThreadId;

	Release();
	Init();

	m_hWnd = hWnd;
	if(!PathFileExists (const_cast<LPCSTR>(pszFileName)))
	{
        memcpy(m_szFileName, pszFileName, MAX_PATH);
        if (StrStrI(m_szFileName, "udp"))
        {
            udp_ = true;
        }
        else 
        {
            MessageBox(m_hWnd, "This file is not exist", "Play", MB_ICONINFORMATION);
            return FALSE;
        }
	}
	else{
		memcpy(m_szFileName, pszFileName, MAX_PATH);
	}

    if (udp_)
    {
        thread_ = CreateThread(NULL, 0, (unsigned long(__stdcall *)(void *))RecvStreamThread, this, 0, &hThreadId);
        SetThreadPriority(thread_, THREAD_PRIORITY_BELOW_NORMAL);
        ResumeThread(thread_);
    }

    //Sleep(100);
	m_nPlayingStatus = ePlaying;
	m_hThread = CreateThread(NULL, 0, (unsigned long(__stdcall *)(void *))DecodeThread, this, 0, &hThreadId);
	SetThreadPriority(m_hThread, THREAD_PRIORITY_BELOW_NORMAL);
	ResumeThread(m_hThread);

    return TRUE;
}

void CLibmadDecoder::Resume()
{
	if(m_nPlayingStatus == ePaused)
	{
		m_nPlayingStatus = ePlaying;
		if(m_hEventPause){
			SetEvent(m_hEventPause);
		}
	}
}

void CLibmadDecoder::Pause()
{
	if(m_nPlayingStatus == ePlaying)
	{
		m_nPlayingStatus = ePaused;
		if(m_hEventPause){
			ResetEvent(m_hEventPause);
		}
	}
}

void CLibmadDecoder::Stop()
{
	m_nPlayingStatus = eStoped;
}

void CLibmadDecoder::SetWaveFormat(unsigned short nChannels, unsigned long nSamplesPerSec, unsigned short wBitsPerSample)
{
	memset(&m_format, 0, sizeof(WAVEFORMATEX));
	m_format.cbSize = 0;
	m_format.nChannels = nChannels;
	m_format.wFormatTag	= WAVE_FORMAT_PCM;
	m_format.nSamplesPerSec	= nSamplesPerSec;
	m_format.wBitsPerSample	= wBitsPerSample;
	m_format.nBlockAlign = m_format.nChannels * (m_format.wBitsPerSample / 8);
	m_format.nAvgBytesPerSec = m_format.nSamplesPerSec * m_format.nBlockAlign;
}

BOOL CLibmadDecoder::OpenDevice()
{
	MMRESULT mmResult;

	if(waveOutGetNumDevs() == 0)
	{
		MessageBox(m_hWnd, "Can't find waveform-audio device", "OpenDeivce", MB_ICONERROR);
		return FALSE;
	}

	mmResult = waveOutOpen(&m_hWaveOut, 
		WAVE_MAPPER, 
		&m_format, 
		(DWORD)waveOutProc, 
		NULL, 
		CALLBACK_FUNCTION
	);
	if(mmResult != MMSYSERR_NOERROR)
	{
		MessageBox(m_hWnd, "Open waveform-audio output device failed", "OpenDeivce", MB_ICONERROR);
		return FALSE;
	}
	else
	{
		m_bOpened = TRUE;
	}

	return TRUE;
}

unsigned int CLibmadDecoder::WriteDevice(struct PCM_BLOCK *pFrame)
{
	MMRESULT mmResult;
	mmResult = waveOutPrepareHeader(m_hWaveOut, &pFrame->pWaveHdr, sizeof(WAVEHDR));
	mmResult = waveOutWrite(m_hWaveOut, &pFrame->pWaveHdr, sizeof(WAVEHDR));
	return mmResult;
}

signed int CLibmadDecoder::LoadFile2Memory(const char *filename, char **result)
{
    if (*result)
    {
        free(*result);
        *result = NULL;
    }
    if (udp_)
    {
        while (recv_ && size_ < max_buffer / 4)
        {
            Sleep(1);
        }

        EnterCriticalSection(&m_cs);
        if (0 == size_)
        {
            LeaveCriticalSection(&m_cs);
            return 0;
        }
        int ret = size_;
        *result = (char *)malloc(size_+1);
        memcpy(*result, &(buffer_), size_);
        size_ = 0;
        LeaveCriticalSection(&m_cs);
        return ret;
    }

	unsigned int size = 0;
    if (NULL == fp_)
    {
        fp_ = fopen(filename, "rb");
        if (fp_ == NULL)
        {
            *result = NULL;
            return 0;
        }
        fseek(fp_, 0, SEEK_END);
        length_ = ftell(fp_);
        fseek(fp_, 0, SEEK_SET);
    }
    size = 4096;
    if (length_ < read_+size)
    {
        size = length_ - read_;
        if (0 == size)
        {
            return 0;
        }
    }
    fseek(fp_, read_, SEEK_SET);
    read_ += size;

    *result = (char *)malloc(size + 1);
	if(size != fread(*result, sizeof(char), size, fp_))
	{
		free(*result);
		return 0;
	}
	//fclose(fp_);
	(*result)[size] = 0;

	return size;
}

int CLibmadDecoder::StartRecvStream()
{
    SOCKET sock = INVALID_SOCKET;
//     FILE * fp = fopen("xxxx.mp3", "wb+");
    while (udp_)
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
            while (max_buffer - size_ < max_buffer / 2)
            {
                Sleep(1);
                continue;
            }
            EnterCriticalSection(&m_cs);
            nRet = ::recvfrom(sock, (char *)buffer_+size_, 4096, 0, &from, &len);
            if (nRet <= 0)
            {
                DWORD dw = GetLastError();
                LeaveCriticalSection(&m_cs);
                break;
            }
            size_ += nRet;
            LeaveCriticalSection(&m_cs);
//             if (fp)
//             {
//                 fwrite(buffer_, size_, 1, fp);
//                 size_ = 0;
//             }
        }
        shutdown(sock, SD_BOTH);
        closesocket(sock);
    }
//     fclose(fp);

    return 0;
}
// 
// 
// int mp3_decode_buf(char *input_buf, int size)
// {
//     int decode_over_flag = 0;
//     int remain_bytes = 0;
//     int ret_val = 0;
//     mad_stream_buffer(&decode_stream, input_buf, size);
//     decode_stream.error = MAD_ERROR_NONE;
//     while (1)
//     {
//         if (decode_stream.error == MAD_ERROR_BUFLEN) {
//             if (decode_stream.next_frame != NULL) {
//                 remain_bytes = decode_stream.bufend - decode_stream.next_frame;
//                 memcpy(input_buf, decode_stream.next_frame, remain_bytes);
//                 return remain_bytes;
//             }
//         }
//         ret_val = mad_frame_decode(&decode_frame, &decode_stream);
//         /* 省略部分代码 */
//             if (ret_val == 0) {
//                 if (play_frame(&decode_frame) == -1) {
//                     return -1;
//                 }
//             }
//         /* 后面代码省略 */
//         ...
//     }
// 
//     return 0;
// }
// 
