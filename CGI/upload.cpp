#include "CGI.hpp"

bool CGI::upload(const HTTPRequest &request, const Config &config)
{
    this->status = 200;
    std::string location_s = request.getInLocation();
    if (request.getPath() == "/favicon.ico")
        return false;
    std::vector<BodyPart> body = request.getBodyParts();
    std::string uploadDir;
    if (body.empty())
        return false;
    bool success = false;
    if (!location_s.empty())
    {
        std::vector<Config::Location> locations = config.getLocations();
        Config::Location location = config.getLocation(location_s);
        if (location.getPath().empty())
            return (print_message("Location not found in upload", RED), status = 404, false);
        std::vector<std::string> upload = location.getUploadDir();
        if (upload.empty())
            return false;
        else
            uploadDir = upload[0];
        std::vector<std::string> methods = location.getAllowMethods();
        std::vector<std::string>::iterator it = std::find(methods.begin(), methods.end(), "POST");
        if (it == methods.end())
            return (print_message("You cannot upload files if POST method is not allowed", RED), status = 405, false);
    }
    struct stat info;
    if (stat(uploadDir.c_str(), &info) != 0 || !(info.st_mode & S_IFDIR))
        return (print_message("Upload directory does not exist", RED), status = 500, false);
    for (std::vector<BodyPart>::const_iterator it = body.begin(); it != body.end(); ++it)
    {
        if (it->is_file == 0)
            continue;
        else
        {
            std::string filename = it->filename;
            size_t lastSlash = filename.find_last_of('/');
            if (lastSlash != std::string::npos)
                filename = filename.substr(lastSlash + 1);
            if (filename.empty())
                continue;
            std::string filePath = uploadDir + "/" + filename;
            std::ofstream outFile(filePath.c_str(), std::ios::out | std::ios::trunc);
            if (!outFile)
                continue;
            const std::string &fileContent = it->data;
            outFile << fileContent;
            if (outFile.good())
                success = true;
            outFile.close();
        }
    }
    if (success)
        print_message("File uploaded successfully", GREEN);
    else
    {
        print_message("File upload failed", RED);
        status = 500;
        return false;
    }
    return success;
}

int CGI::getStatus() const { return status; }