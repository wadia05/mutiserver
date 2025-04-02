#include "MimeTypes.hpp"

MimeTypes::MimeTypes(const std::string &path)
{
	std::ifstream file(path.c_str());
	if (!file.is_open())
	{
		print_message("Error: Could not open mimeTypes file", RED);
		exit(1);
	}
	std::string line;
	while (std::getline(file, line))
	{
		std::istringstream ss(line);
		std::string extension, mimeType;
		if (!(std::getline(ss, extension, ',') && std::getline(ss, mimeType)))
		{
			print_message("Error: Invalid line format in mimeTypes file", RED);
			file.close();
			exit(1);
		}
		_mimeTypes[extension] = mimeType;
	}
	file.close();
}

MimeTypes::~MimeTypes() { _mimeTypes.clear(); }

std::string MimeTypes::getMimeType(const std::string &extension)
{
	std::map<std::string, std::string>::iterator it = _mimeTypes.find(extension);
	if (it == _mimeTypes.end())
		return "text/plain";
	return it->second;
}

void MimeTypes::printMimeTypes()
{
	for (std::map<std::string, std::string>::iterator it = _mimeTypes.begin(); it != _mimeTypes.end(); ++it)
		print_message(it->first + " " + it->second, CYAN);
}