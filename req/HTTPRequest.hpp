#pragma once
#include "../main.hpp"
class Config;

void trim(std::string &str);
bool isHex(char c);
std::string urlDecode(const std::string &encoded);
class Location;
struct BodyPart
{
    int is_file;
    std::string data;
    std::string name;
    std::string filename;
    std::string type_of_file;
    std::string content_type;
};

class HTTPRequest
{
private:
    std::string method;
    std::string path;
    std::string http_version;
    std::map<std::string, std::string> query_params;
    std::map<std::string, std::string> headers;
    int is_multi_part;
    std::vector<BodyPart> body_parts;
    std::string all_body;
    std::string content_type;
    std::string in_location;
    bool is_redirect;
    int status;

public:
    bool hasHeader(const std::string &name) const;
    std::string getHeader(const std::string &name) const;
    void parseQueryString(const std::string &query_string);
    void parseURLEncodedBody(std::string body, const std::string &CT);
    void parseRawBody(std::string body, const std::string &CT);
    void parseMultipartBody(std::string body, const std::string &CT);
    void parsePart(const std::string &part_content);

    bool parse_request(const std::string &request, const Config &config);
    bool parseRequestLine(const std::string &line, const Config &config);
    bool parseHeaderLine(const std::string &line);
    bool parseBody(std::istringstream &iss);

    std::string getMethod() const;
    std::string getPath() const;
    std::string getHttpVersion() const;
    int getStatusCode() const;
    bool isRedirect() const;
    const std::map<std::string, std::string> &getQueryParams() const;
    const std::map<std::string, std::string> &getHeaders() const;
    const std::vector<BodyPart> &getBodyParts() const;
    std::string getBodyContent() const;
    std::string getContentType() const;
    int getIsMultiPart() const;
    void print_all();
    std::string getInLocation() const;
};
