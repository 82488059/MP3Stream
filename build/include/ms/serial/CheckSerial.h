#pragma once
#include <string>
#include <ms/tstring.h>
namespace ms
{
    // registryMainkey 注册表项
    // keyname 加密关键字
    // registryValuekey 注册表值
    bool CheckSerial(const ms::tstring& registryMainkey, const ms::tstring& keyname = _T(""), const ms::tstring&registryValuekey = _T("RegisterSerial"));
    //bool Check(const std::string& serial, const std::string& key);
}
