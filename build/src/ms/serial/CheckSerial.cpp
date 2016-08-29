#include "stdafx.h"

#include <ms/serial/CheckSerial.h>

#include <Windows.h>

#include <ms/serial/MakeSerial.h>
#include <ms/dev/GetHDSerial.h>
#include <ms/reg/registry.h>
#include <iostream>
#include <memory>
#include <functional>
#include <ms/log/LogToFile.h>

namespace ms
{
    bool Check(const std::string& serial, const std::string& key)
    {
        static ms::CMakeSerial makeSerial(dev::GetHDSerial() + key);
        AppErrorLogFormat(_T("硬盘号%s"), dev::GetHDSerial().c_str());
        return  makeSerial.GetSerial() == serial;
    }

    std::string GetRegSerial(const ms::tstring& registryMainkey, const ms::tstring&registryValuekey)
    {
        TCHAR szRegisterSerial[MAX_PATH];
        reg::CRegistry registry(registryMainkey.c_str());
        if (registry.QueryValue(registryValuekey.c_str(), szRegisterSerial, MAX_PATH))
        {
            AppErrorLogFormat(_T("读取序列号成功！%s"), szRegisterSerial);
#ifdef _UNICODE
            DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, szRegisterSerial
                , -1, NULL, 0, NULL, FALSE);

            std::shared_ptr<char> pDelete(new char[dwNum]
                , [](char* psText){delete[] psText; }
            );
            if (!pDelete)
            {
                return "";
            }
            WideCharToMultiByte(CP_OEMCP, NULL, szRegisterSerial, -1
                , pDelete.get(), dwNum, NULL, FALSE);
            return pDelete.get();
#else
            return szRegisterSerial;
#endif // _UNICODE
        }
        return "";
    }

    bool CheckSerial(const ms::tstring& registrykey, const ms::tstring& keyname, const ms::tstring&registryValuekey)
    {
        ms::tstring& registryMainkey = _T("SOFTWARE\\") + registrykey;
#ifdef _UNICODE
        DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, keyname.c_str()
            , -1, NULL, 0, NULL, FALSE);

        std::shared_ptr<char> pDelete(new char[dwNum]
            , [](char* psText){delete[] psText; }
        );
        if (!pDelete)
        {
            return false;
        }
        WideCharToMultiByte(CP_OEMCP, NULL, keyname.c_str(), -1
            , pDelete.get(), dwNum, NULL, FALSE);
        std::string key = pDelete.get();

#else
        std::string key = keyname;
#endif // _UNICODE

        while (true)
        {
            std::string reg_serial = GetRegSerial(registryMainkey, registryValuekey);
            if (!reg_serial.empty())
            {
                if (Check(reg_serial, key))
                {
                    AppErrorLog(_T("验证序列号成功！"));
                    std::cout << "验证序列号成功！" << std::endl;
                    return true;
                }
                else
                {
                    AppErrorLog(_T("验证序列号失败！"));
                    std::cout << "验证序列号失败！"  << std::endl;
                }
            }
            std::cout << "硬盘序列号：" << dev::GetHDSerial() << std::endl;
            std::cout << "请输入注册码:";
#ifdef _UNICODE
            std::wstring serial;
            std::wcin >> serial;
#else
            std::string serial = "";
            std::cin >> serial;
#endif // _UNICODE
            reg::CRegistry registry(registryMainkey.c_str());
            if (registry.SetValue(serial.c_str(), registryValuekey.c_str()))
            {
                AppErrorLog(_T("写入序列号成功！"));
                std::cout << "写入序列号成功" << std::endl;
                continue;
            }
            else
            {
                AppErrorLog(_T("写入序列号失败！请以系统管理员身份启动。"));
                std::cout << "写入序列号失败！请以系统管理员身份启动。" << std::endl;
                return false;
            }
        }
        return false;
    }




}

