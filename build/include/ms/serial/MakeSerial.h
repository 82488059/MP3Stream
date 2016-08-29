#pragma once
#include <string>

namespace ms{
class CMakeSerial
{
    CMakeSerial();
public:
	CMakeSerial(const std::string& hdSerial);
	~CMakeSerial();
	void Update(const std::string& hdSerial);
	std::string GetSerial();

	void MakeSerial();


private:
	bool _finished;
	std::string _hdSerial;
	std::string _serial;

};

}