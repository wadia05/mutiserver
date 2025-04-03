#include "connectionHandeling.hpp"

Connection::Connection(int fd) : fd(fd), status_code(200),is_redection(false) , read_buffer(""), write_buffer(""), response(""), path(""), readFormFile(NULL), query(""), upload_path(""), method(NOTDETECTED),
                                 last_active(time(NULL)), content_length(0), total_sent(0),total_received(0), keep_alive(true), headersSend(false), chunked(false),
                                    state(IDLE), is_cgi(false)
{
    readFormFile = new std::ifstream(); // Initialize the pointer
    std::cout << GREEN << this->content_length << RESET << std::endl;
}
Connection::~Connection()
{
    if (readFormFile)
    {
        if (readFormFile->is_open())
        {
            readFormFile->close();
        }
        delete readFormFile;
    }
}
std::string Connection::GetHeaderResponse()
{
    // (void)status_code;
    std::string contentType;

    // Determine content type based on file extension
    std::string extension;
    size_t dotPos = this->path.find_last_of('.');
    if (dotPos != std::string::npos)
    {
        extension = this->path.substr(dotPos + 1);
    }

    // Set content type based on extension
    if (extension == "html" || extension == "htm")
    {
        contentType = "text/html";
    }
    else if (this->is_cgi)
    {
        contentType = "text/html";
    }
    else if (extension == "css")
    {
        contentType = "text/css";
    }
    else if (extension == "js")
    {
        contentType = "application/javascript";
    }
    else if (extension == "jpg" || extension == "jpeg")
    {
        contentType = "image/jpeg";
    }
    else if (extension == "png")
    {
        contentType = "image/png";
    }
    else if (extension == "gif")
    {
        contentType = "image/gif";
    }
    else if (extension == "svg")
    {
        contentType = "image/svg+xml";
    }
    else if (extension == "json")
    {
        contentType = "application/json";
    }
    else if (extension == "pdf")
    {
        contentType = "application/pdf";
    }
    else if (extension == "txt")
    {
        contentType = "text/plain";
    }
    else if (extension == "mp3")
    {
        contentType = "audio/mpeg";
    }
    else if (extension == "wav")
    {
        contentType = "audio/wav";
    }
    else if (extension == "ogg")
    {
        contentType = "audio/ogg";
    }
    else if (extension == "mp4")
    {
        contentType = "video/mp4";
    }
    else if (extension == "webm")
    {
        contentType = "video/webm";
    }
    else if (extension == "avi")
    {
        contentType = "video/x-msvideo";
    }
    else
    {
        contentType = "application/octet-stream";
    }
    // Build HTTP response header
    std::stringstream ss;
    ss << "HTTP/1.1 " << this->status_code << " " << GetStatusMessage() << "\r\n";
    if (this->is_redection == true)
    {
        ss << "Location: " <<  this->response << "\r\n";
        this->is_redection = false;
        this->response.clear();

    }
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << (this->is_cgi ? this->response.size() : this->content_length) << "\r\n";
    ss << "Server: MoleServer\r\n";
    ss << "Connection: " << (this->keep_alive ? "keep-alive" : "close") << "\r\n";
    ss << "\r\n"; // Empty line to separate headers from body

    return ss.str();
}

void Connection::GetBodyResponse()
{
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    this->readFormFile->read(buffer, BUFFER_SIZE);
    this->write_buffer.append(buffer);
    // std::cout << "Buffer: " << buffer << std::endl;

    if (this->readFormFile->eof())
    {
        this->readFormFile->close();
        // this->is_writing = false;
        // // this->is_parsing = false;
        // this->is_possessing = false;
        // this->is_reading = true;
        // mod_epoll(this->fd, EPOLLIN);
    }
}

std::string Connection::GetStatusMessage()
{
    switch (this->status_code)
    {
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 204:
        return "No Content";
    case 413:
        return "Payload Too Large";
    case 400:
        return "Bad Request";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 302:
        return "Found";
    case 500:
        return "Internal Server Error";
    default:

        return "Unknown";
    }
}


void Connection::GetStateFilePath()
{

    if (this->path.empty())
    {
        switch (this->status_code)
        {
        case 200:
            this->path = "www/indexx.html";
            break;
        case 201:
            this->path = "www/suss/postsuss.html";
            break;
        case 204:
            this->path = "www/suss/deletesuss.html";
            break;
        case 413:
            this->path = "www/error_pages/413.html";
            break;
        case 400:
            this->path = "www/error_pages/400.html";
            break;
        case 403:
            this->path = "www/error_pages/403.html";
            break;
        case 404:
            this->path = "www/error_pages/404.html";
            break;
        case 500:
            this->path = "www/error_pages/500.html";
            break;
        default:
            this->path = "www/error_pages/default.html";
            break;
        }
    }

    this->readFormFile->open(this->path.c_str(), std::ios::in | std::ios::binary);
    std::cout << "Path: " << this->path << std::endl;
    if (!this->readFormFile->is_open())
    {
        std::cerr << "Failed to open file" << std::endl;
        this->path = "www/error_pages/default.html";
        this->readFormFile->open(this->path.c_str(), std::ios::in | std::ios::binary);
        if (!this->readFormFile->is_open())
        {
            std::cerr << "Failed to open error file" << std::endl;
            return ;
        }
    }
    struct stat st;
    stat(this->path.c_str(), &st);
    this->content_length = st.st_size;
    

    return ;
}

std::string Connection::GetContentType()
{
    std::string extension = this->path.substr(this->path.find_last_of(".") + 1);
    if (extension == "html" || extension == "htm")
    {
        return "text/html";
    }
    else if (extension == "css")
    {
        return "text/css";
    }
    else if (extension == "js")
    {
        return "application/javascript";
    }
    else if (extension == "jpg" || extension == "jpeg")
    {
        return "image/jpeg";
    }
    else if (extension == "png")
    {
        return "image/png";
    }
    else if (extension == "gif")
    {
        return "image/gif";
    }
    else if (extension == "svg")
    {
        return "image/svg+xml";
    }
    else if (extension == "json")
    {
        return "application/json";
    }
    else if (extension == "pdf")
    {
        return "application/pdf";
    }
    else if (extension == "txt")
    {
        return "text/plain";
    }
    else if (extension == "mp3")
    {
        return "audio/mpeg";
    }
    else if (extension == "wav")
    {
        return "audio/wav";
    }
    else if (extension == "ogg")
    {
        return "audio/ogg";
    }
    else if (extension == "mp4")
    {
        return "video/mp4";
    }
    else if (extension == "webm")
    {
        return "video/webm";
    }
    else if (extension == "avi")
    {
        return "video/x-msvideo";
    }
    else
    {
        return "application/octet-stream";
    }
}
