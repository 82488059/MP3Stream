#pragma once
class CPcmPlay
{
public:
    CPcmPlay();
    ~CPcmPlay();

    enum PLAYING_STATUS
    {
        ePlaying = 0,
        ePaused,
        eStoped
    };
};

