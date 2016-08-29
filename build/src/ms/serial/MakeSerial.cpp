#include "stdafx.h"
#include "ms/serial/MakeSerial.h"
#include <ms/md5/md5.h>
#include <ms/sha/sha1.h>


namespace ms{

CMakeSerial::CMakeSerial(const std::string& hdSerial)
: _finished(false)
, _hdSerial(hdSerial)
, _serial("")
{

}

CMakeSerial::~CMakeSerial()
{


}

std::string CMakeSerial::GetSerial()
{
	if (_finished)
	{
		return _serial;
	}
	MakeSerial();
	return _serial;
}
void CMakeSerial::MakeSerial()
{
	std::string str = _hdSerial;
	MD5 md5(str);
	std::string szMd5 = md5.toString();
	SHA1 sha1;
	char buffer[41];
	sha1.SHA_GO(str.c_str(),buffer);
	// std::string szSha1 = buffer;
	md5.update(buffer, 40);
	sha1.SHA_GO(szMd5.c_str(), buffer);
	std::string tmd5 = md5.toString();
	std::string t1;
	t1.reserve(72);
	for (int i = 0; i < 8; ++i)
	{
		t1.append(1, buffer[i*5]);
		for (int j = 0; j < 4; ++j)
		{
			t1.append(1, tmd5[i*4+j]);
			t1.append(1, buffer[i*5+j+1]);
		}
	}
	sha1.SHA_GO(t1.c_str(), buffer);
	md5.update(buffer);
	_serial = md5.toString().c_str();
}
void CMakeSerial::Update(const std::string& hdSerial)
{
	_finished = false;
	_hdSerial = hdSerial;
}

}
