// LibmadDecoder.h: interface for the CLibmadDecoder class.
//
//////////////////////////////////////////////////////////////////////
#pragma once
#include <WinSock2.h>

#include "mad.h"
// #pragma comment(lib, "libmad.lib")
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <MMSystem.h>
#pragma comment(lib, "winmm.lib")

#include <stdio.h>

#define DOUBLE_BUFFER 2
#define MAX_RESAMPLEFACTOR (6)
#define MAX_NSAMPLES (1152 * MAX_RESAMPLEFACTOR) /* 1152 because that's what mad has as a max */
#define MAX_OUTPUTBUFFER (48000 * 4 * 2)

class CLibmadDecoder  
{
public:
	CLibmadDecoder();
	virtual ~CLibmadDecoder();

	enum PLAYING_STATUS
	{
		ePlaying = 0,
		ePaused,
		eStoped
	};

	//
	// This is a private message structure. A generic pointer to this structure
	// is passed to each of the callback functions. Put here any data you need
	// to access from within the callbacks
	//
	struct BUFFER
	{
		BUFFER()
		{
			data = NULL;
			length = 0;
		}
		~BUFFER()
		{
			if(data)
			{
				free((void*)data);
				data = NULL;
			}
			length = 0;
		}
		unsigned char *data;
		unsigned long length;
	};

	struct PCM_BLOCK 
	{
		PCM_BLOCK()
		{
			memset(&pWaveHdr, 0, sizeof(pWaveHdr));
			bIsPlaying = 0;
			hPlayEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		}
		
		~PCM_BLOCK()
		{
			if(hPlayEvent!=NULL && hPlayEvent != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hPlayEvent);
			}
			hPlayEvent = NULL;
		}
		
		WAVEHDR pWaveHdr;
		BOOL bIsPlaying;
		HANDLE hPlayEvent;
		
		void release()
		{
			if(pWaveHdr.lpData)
			{
				free(pWaveHdr.lpData);
			}
			pWaveHdr.lpData = NULL;
			bIsPlaying = FALSE;
		}
	};

public:

	// Callback functions for decoder
	enum mad_flow static input_func(void *data, struct mad_stream *stream);
	enum mad_flow static output_func(void *data, struct mad_header const *header, struct mad_pcm *pcm);
	enum mad_flow static header_func(void *data, struct mad_header const*);
	enum mad_flow static filter_func(void *data, struct mad_stream const*stream, struct mad_frame* frame);
	enum mad_flow static error_func(void *data, struct mad_stream *stream, struct mad_frame *frame);

	// Play pause and stop
	BOOL Play(char* pszFileName, HWND hWnd = NULL);
	void Resume();
	void Pause();
	void Stop();
	BOOL IsPlaying(){return (m_nPlayingStatus == ePlaying);}
	BOOL IsPaused(){return (m_nPlayingStatus == ePaused);}

	// Init local variables
	void Init();

	// Release buffers
	void Release();	

	// Starting decode
	int StartDecode();

private:

	// Simple rounding, clipping, and scaling of MAD's high-resolution samples down to specified bits
	signed int Scale(unsigned int bits, mad_fixed_t sample);

	// Write a block of signed 16-bit little-endian PCM samples
	unsigned int CombinePCM16(unsigned char *data, unsigned int nsamples, mad_fixed_t const *left, mad_fixed_t const *right);

	// Write a block of signed 24-bit little-endian PCM samples
	unsigned int CombinePCM24(unsigned char *data, unsigned int nsamples, mad_fixed_t const *left, mad_fixed_t const *right);

	// Load a mp3 file to memory
	int LoadFile2Memory(const char *filename, char **result);

	// Open d default waveform-audio device
	BOOL OpenDevice();

	// Set wave format
	void SetWaveFormat(unsigned short nChannels, unsigned long nSamplesPerSec, unsigned short wBitsPerSample);

	// Write PCM data to waveform-audio device
	unsigned int WriteDevice(struct PCM_BLOCK *pFrame);

	friend void WINAPI DecodeThread(CLibmadDecoder *pLibmadDecoder);
	friend void CALLBACK waveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);

private:
	struct mad_decoder m_Decoder;
	PCM_BLOCK m_PcmBlock[DOUBLE_BUFFER];
	char m_szFileName[MAX_PATH];
	HWND m_hWnd;
	HANDLE m_hThread;
	volatile HANDLE m_hEventPause;
	BOOL m_bOpened;
	int m_nBufIndex;
	BUFFER m_outBuffer;
	unsigned char const *m_pFileBuffer;
	unsigned long m_nFilebufferLen;
	HWAVEOUT m_hWaveOut;
	WAVEFORMATEX m_format;
	volatile enum PLAYING_STATUS m_nPlayingStatus;
    FILE* fp_;
    DWORD read_;
    DWORD length_;
    HANDLE thread_;
    friend void WINAPI RecvStreamThread(CLibmadDecoder *pLibmadDecoder);
    int StartRecvStream();
    enum{max_buffer = 4096, max_top = 50};
    volatile int top_;
    volatile int cur_top_;
    char buffer_[max_top][max_buffer];
    int size_[max_top];
    CRITICAL_SECTION m_cs;
    bool udp_;
    volatile bool recv_;
};
