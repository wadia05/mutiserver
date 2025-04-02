#pragma once

#include "../main.hpp"

class MimeTypes
{
private:
	std::map<std::string, std::string> _mimeTypes;

public:
	MimeTypes(const std::string &path);
	~MimeTypes();
	std::string getMimeType(const std::string &extension);
	void printMimeTypes();
};