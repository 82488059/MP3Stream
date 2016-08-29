#pragma once
#include <string>

namespace ms{

#ifdef _UNICODE
typedef std::wstring string;
#else
typedef std::string string;
#endif // _UNICODE
}
