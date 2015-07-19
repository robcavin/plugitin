#include <Windows.h>
#include <sstream>
#include <iomanip>

template <typename T> std::string asHex(T i) {
	std::stringstream stream;
	stream << "0x" << std::setfill('0') << std::setw(sizeof(T) * 2) << std::hex << i;
	return stream.str();
}

void error(const std::string& message) {
	std::stringstream stream;
	stream << message << "\n";
	OutputDebugStringA(stream.str().c_str());
	exit(1);
}

void errorLastError(const std::string& message) {
	std::stringstream stream;
	DWORD error = GetLastError();
	stream << message << "failed with " << asHex(error) << "(" << error << ")\n";
	OutputDebugStringA(stream.str().c_str());
	exit(1);
}

size_t readFile(char*& buffer, const char* fileName) {
	std::stringstream stream;
	HANDLE file =
		CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file == INVALID_HANDLE_VALUE) error(std::string("CreateFileA failed to open \"") + fileName + "\"");
	LARGE_INTEGER size;
	if (!GetFileSizeEx(file, &size)) errorLastError("GetFileSizeEx");
	buffer = new char[size.QuadPart];
	DWORD out;
	if (!ReadFile(file, buffer, size.LowPart, &out, NULL) || out != size.QuadPart) errorLastError("ReadFile");
	CloseHandle(file);
	return (size_t)size.QuadPart;
}


void validateHR(HRESULT hr, const std::string& message) {
	if (SUCCEEDED(hr)) return;
	std::stringstream stream;
	stream << message << "failed with " << asHex(hr) << "(" << hr << ")\n";
	OutputDebugStringA(stream.str().c_str());
	exit(1);
}