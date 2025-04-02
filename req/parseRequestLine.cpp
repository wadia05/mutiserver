#include "HTTPRequest.hpp"

bool isValidPath(const std::string &path, bool isDirectory)
{
    struct stat path_stat;
    if (stat(path.c_str(), &path_stat) != 0)
        return false;
    if (isDirectory)
    {
        DIR *dir = opendir(path.c_str());
        if (dir)
        {
            closedir(dir);
            return true;
        }
        return false;
    }
    return S_ISREG(path_stat.st_mode);
}

void HTTPRequest::parseQueryString(const std::string &query_string)
{
    std::istringstream iss(query_string);
    std::string key_value;
    while (std::getline(iss, key_value, '&'))
    {
        trim(key_value);
        size_t pos = key_value.find('=');
        if (pos != std::string::npos)
        {
            std::string key = key_value.substr(0, pos);
            std::string value = key_value.substr(pos + 1);
            trim(key);
            trim(value);
            query_params[urlDecode(key)] = urlDecode(value);
        }
        else
            query_params[urlDecode(key_value)] = "";
    }
}

bool HTTPRequest::parseRequestLine(const std::string &line, const Config &config)
{
    std::istringstream iss(line);
    if (!(iss >> method >> path >> http_version))
        return (print_message("Invalid request: " + line, RED), status = 400, false);
    trim(method);
    trim(path);
    trim(http_version);
    if (method != "GET" && method != "POST" && method != "DELETE")
        return (print_message("Invalid request method: " + method, RED), status = 405, false);
    if (http_version != "HTTP/1.1")
        return (print_message("Invalid HTTP version: " + http_version, RED), status = 505, false);
    size_t pos = path.find("?");
    if (pos != std::string::npos)
    {
        parseQueryString(path.substr(pos + 1));
        path = path.substr(0, pos);
    }
    path = urlDecode(path);
    if (path.empty() || path[0] != '/')
        return (print_message("Invalid path: " + path, RED), status = 404, false);
    if (path == "/favicon.ico")
        return true;
    if (path[0] == '/')
        path = path.substr(1);
    const std::vector<Config::Location> &locations = config.getLocations();
    std::string new_path = path;
    std::string default_root = config.getDefaultRoot()[0];
    if (!default_root.empty() && default_root[default_root.size() - 1] != '/')
        default_root += "/";
    if (new_path.empty())
    {
        new_path = default_root;
        if (new_path.empty())
            return (print_message("No default root specified", RED), status = 500, false);
    }
    if (new_path.find("/") == std::string::npos)
        new_path = default_root + new_path;
    else
    {
        std::string file;
        std::string dir;
        size_t pos = new_path.find_last_of("/");
        if (pos != std::string::npos)
        {
            file = new_path.substr(pos + 1);
            dir = new_path.substr(0, pos);
        }
        else
        {
            file = new_path;
            dir = default_root;
        }
        for (std::vector<Config::Location>::const_iterator it = locations.begin(); it != locations.end(); ++it)
        {
            size_t pos = it->getPath().find(dir);
            if (pos != std::string::npos)
            {
                new_path = it->getPath() + file;
                break;
            }
        }
    }
    if (isValidPath(new_path, false))
    {
        std::string file;
        std::string dir;
        size_t pos = new_path.find_last_of("/");
        int i = 0;
        if (pos != std::string::npos)
        {
            file = new_path.substr(pos + 1);
            dir = new_path.substr(0, pos);
            i = 1;
        }
        else
        {
            file = new_path;
            dir = default_root;
        }
        if (i == 1)
            dir += "/";
        Config::Location location = config.getLocation(dir);
        if (location.getPath().empty())
            return (print_message("Path not found in locations: " + dir, RED), status = 404, false);
        this->in_location = dir;
        path = new_path;
        std::vector<std::string> methods = location.getAllowMethods();
        if (!methods.empty() && std::find(methods.begin(), methods.end(), method) == methods.end())
            return (print_message("Method not allowed: " + method, RED), status = 405, false);
    }
    else if (isValidPath(new_path, true))
    {
        Config::Location location = config.getLocation(new_path);
        if (location.getPath().empty())
            return (print_message("Path not found in locations: " + new_path, RED), status = 404, false);
        this->in_location = new_path;
        std::vector<std::string> index = location.getIndex();
        if (index.empty())
            index = config.getDefaultIndex();
        std::string index_file = new_path + index[0];
        if (!isValidPath(index_file, false))
            return (print_message("Invalid index file: " + index_file, RED), status = 404, false);
        path = index_file;
        std::vector<std::string> methods = location.getAllowMethods();
        if (!methods.empty() && std::find(methods.begin(), methods.end(), method) == methods.end())
            return (print_message("Method not allowed: " + method, RED), status = 405, false);
    }
    else
        return (print_message("Invalid path: " + new_path, RED), status = 404, false);
    return true;
}
