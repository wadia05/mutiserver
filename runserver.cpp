#include "runserver.hpp"

Run::Run(char **av)
{

    // Initialize the server
    MimeTypes mimeTypes("www/mimeTypes.csv");
    std::ifstream file(av[1]);
    Config config;
    config.parseConfig(file);
    configs = config.getConfigs();

    for (size_t i = 0; i < configs.size(); i++)
    {
        for (size_t j = 0; j < configs[i].getPort().size(); j++)
        {
            Server *server = new Server();
            server->setPort(std::atoi(configs[i].getPort()[j].c_str()));
            server->setServerName(configs[i].getServerName()[0]);
            server->setServerIp(configs[i].getHost()[0]);
            server->setuploadSize(configs[i].getClientMaxBodySize()[0]);
            server->setroot(configs[i].getDefaultRoot()[0]);
            server->setconnfig_index(i);
            std::cout << "here " << server[i].getconnfig_index() << std::endl;
            servers.push_back(server);
        }
    }

    epoll_fd = epoll_create(1); // Use the class member epoll_fd
    if (epoll_fd < 0)
    {
        throw std::runtime_error("Failed to create epoll");
    }
    this->printrunservers();
    this->connections.resize(servers.size());
    this->createServer();
}
#include <fstream>

void test_dir(std::string path)
{
    std::ofstream htmlFile("file_list.html");
    if (!htmlFile)
    {
        throw std::runtime_error("Failed to create HTML file");
    }

    htmlFile << "<html><body>\n";
    htmlFile << "<h1>Files and Directories in " << path << "</h1>\n";

    DIR *dir;
    struct dirent *entry;

    dir = opendir(path.c_str());
    if (dir == NULL)
    {
        throw std::runtime_error("Failed to open directory");
    }
  
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR)
        {
            htmlFile << "<a href=\"" << path + entry->d_name << "\">" << entry->d_name << "</a><br>\n";
        }
        else if (entry->d_type == DT_REG)
        {
            htmlFile << "<a href=\"" << path + entry->d_name << "\">" << entry->d_name << "</a><br>\n";
        }
    }

    closedir(dir);

    htmlFile << "</body></html>\n";
    htmlFile.close();
}
void Run::createServer()
{
    test_dir("www/suss");
    for (size_t i = 0; i < servers.size(); i++)
    {
         // Create server socket
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0)
        {
            throw std::runtime_error("Failed to create socket");
        }
        servers[i]->setserverfd(server_fd);
        // Set socket options
        int opt = 1;
        if (setsockopt( server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        {
            throw std::runtime_error("Failed to set SO_REUSEADDR");
        }

        // Bind and listen
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(servers[i]->getServerIp().c_str());
        addr.sin_port = htons(servers[i]->getPort());

        if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            std::cout << servers[i]->getPort() << std::endl;
            perror("bind");
            throw std::runtime_error("Failed to bind socket");
        }

        if (listen(server_fd, SOMAXCONN) < 0)
        {
            throw std::runtime_error("Failed to listen");
        }
        setnon_blocking(server_fd);
        // Add server socket to epoll
        add_to_epoll(server_fd, EPOLLIN);
    }
}

bool Run::handleConnection(int fd, int j)
{
    // Accept new connection
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0)
    {
        std::cerr << "Failed to accept connection" << std::endl;
        return false;
    }
    setnon_blocking(client_fd);
    add_to_epoll(client_fd, EPOLLIN);
    this->connections[j][client_fd] = new Connection(client_fd);
    this->connections[j][client_fd]->state = Connection::READING;
    print_message ("New connection accepted", GREEN);
    return true;
}
void Run::readRequest(Connection *conn)
{
    if (conn->fd <= 0) {
        std::cerr << "No socket available" << std::endl;
         conn->state = Connection::CLOSING;
        return;
    }
    if (!conn){
        std::cerr << "Connection object is NULL" << std::endl;
         conn->state = Connection::CLOSING;
        return;
    }
    char buffer[BUFFER_SIZE];
    int read_bytes = recv(conn->fd, buffer, sizeof(buffer), MSG_NOSIGNAL);


    // Check for EAGAIN or EWOULDBLOCK
    if (read_bytes < 0) {
        std::cerr << "Failed to read request" << std::endl;
         conn->state = Connection::CLOSING;
        // close_connection(conn);
        return;
    }

    if (read_bytes == 0) {
        std::cerr << "Client disconnected" << std::endl;
         conn->state = Connection::CLOSING;
        // close_connection(conn);
        return;
    }

    conn->read_buffer.append(buffer, read_bytes);
    // conn->last_active = time(0);

    if (conn->total_received == 0) {
        const std::string content_length_header = "Content-Length: ";
        size_t pos = conn->read_buffer.find(content_length_header);
        if (pos != std::string::npos) {
            size_t start = pos + content_length_header.length();
            size_t end = conn->read_buffer.find("\r\n", start);
            if (end != std::string::npos) {
                try {
                    conn->content_length = std::stoi(conn->read_buffer.substr(start, end - start));
                    std::cout << "Extracted Content-Length: " << conn->content_length << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Error parsing Content-Length: " << e.what() << std::endl;
                    close_connection(conn);
                    return;
                }
            }
        }
    }

    if ((long)conn->content_length > this->servers[this->currIndexServer]->getuploadSize())
    {
        conn->state = Connection::POSSESSING;
        conn->keep_alive = false; // Disable keep-alive for this request
        mod_epoll(conn->fd, EPOLLOUT);
    }
    else if (conn->content_length > 0 && conn->read_buffer.size() >= conn->content_length) {
        conn->state = Connection::POSSESSING;
        conn->keep_alive = false; // Disable keep-alive for this request
        mod_epoll(conn->fd, EPOLLOUT);
        std::cout << "Extracted Content-Length: " << conn->content_length << std::endl;
        std::cout << "Total received: " << conn->total_received << std::endl;
    }else if (conn->content_length == 0) {
        conn->state = Connection::POSSESSING;
        mod_epoll(conn->fd, EPOLLOUT);
    }

}
void setReqType(Connection *conn, HTTPRequest request)
{
    if (conn->method == NOTDETECTED)
    {
        if (request.getMethod() == "GET" && conn->status_code == 200)
            conn->method = GET;
        else if (request.getMethod() == "POST" && conn->status_code == 200)
            conn->method = POST;
        else if (request.getMethod() == "DELETE" && conn->status_code == 200)
            conn->method = DELETE;
    }
}
void Run::possessRequest(Connection *conn, HTTPRequest &request)
{
    // HTTPRequest request = HTTPRequest(conn->read_buffer);
    if (conn->method == GET)
    {
        std::cout << "GET request" << std::endl;
        this->GET_hander(conn, request);
    }
    else if (conn->method == POST)
    {
        std::cout << "POST request" << std::endl;
        this->POST_hander(conn);
    }
    else if (conn->method == DELETE)
    {
        std::cout << "DELETE request" << std::endl;
        this->DELETE_hander(conn, request);
    }
    else
    {
        // conn->status_code = 400;
        std::cout << conn->status_code << std::endl;
        print_message("Unknown request", RED);
    }
    conn->GetStateFilePath();
    conn->state = Connection::WRITING;
}




void Run::parseRequest(Connection *conn)
{
    HTTPRequest request;
    int confidx = this->servers[this->currIndexServer]->getconnfig_index();
    // std::cout <<"config size : " <<this->configs.size() << std::endl;

    // std::cout << "Config index: " << confidx << std::endl;
    if (!request.parse_request(conn->read_buffer,configs[confidx])) {

        
        std::cout << "Failed to parse request" << std::endl;
        // std::cout << "Request: " << conn->read_buffer << std::endl;
        conn->write_buffer.clear();
        // print_message("Error parsing request", RED);
        conn->path = "";
        conn->status_code = request.getStatusCode();
        std::cout << "Status code: " << conn->status_code << std::endl;
    }
    else
    {
        CGI cgi;
        int i = 0;
        bool is_upload = cgi.upload(request, this->configs[confidx]);
        if (!is_upload && cgi.getStatus() != 200)
        {
            conn->status_code = cgi.getStatus();
            return;
        }
        else if (is_upload)
            i = 1;
        if (cgi.is_cgi(request.getPath(), this->configs[confidx], request.getInLocation()) && i == 0)
        {
            if (!cgi.exec_cgi(request, conn->response))
                conn->status_code = cgi.getStatus();
        }

    }
    conn->read_buffer.clear();
    setReqType(conn, request);
    possessRequest(conn, request);
    // request.print_all();
    // conn->last_active = time(0);
    // conn->state = Connection::WRITING;
}
void Run::handleRequest(Connection *conn)
{
    if (!conn)
    {
        std::cerr << "Connection object is NULL" << std::endl;
        return;
    }
    if ( conn->state == Connection::CLOSING)
    {
        std::cout << "Connection is closing by state close" << std::endl;
        close_connection(conn);
        return;
    }
    else if (conn->state == Connection::READING)
    {
        this->readRequest(conn);
        // Connection might be closed in readRequest, check before continuing
        // if (conn->fd <= 0) return;
    }
    else if (conn->state == Connection::POSSESSING)
    {
        this->parseRequest(conn);
        // Connection might be closed in parseRequest, check before continuing
        // if (conn->fd <= 0) return;
    }
    else if (conn->state == Connection::WRITING)
    {
        this->sendResponse(conn);
        // Don't do anything after sendResponse since the connection might be closed
        // return;
    }
    
    // Update the last active time only if we still have a valid connection
    conn->last_active = time(0);
}
void Run::runServer()
{
    struct epoll_event events[MAX_EVENTS];
    if (epoll_fd < 0) // Ensure epoll_fd is valid
    {
        throw std::runtime_error("Epoll file descriptor is not initialized");
    }

    printf("Server is running...\n");
    while (true)
    {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        for (int i = 0; i < num_events; i++)
        {
            int current_fd = events[i].data.fd;
            // std::cout << servers.size() << std::endl;
            for (size_t j = 0; j < servers.size(); j++)
            {
                this->currIndexServer = i % servers.size();
                if (current_fd == this->servers[j]->getserverfd())
                {
                    print_message("Waiting for events...", YELLOW);
                    if (this->handleConnection(current_fd, j) == false)
                        continue;
                    break;
                }
                else
                {

                    std::map<int, Connection *>::iterator it = this->connections[j].find(current_fd);
                    if (it != this->connections[j].end())
                    {
                        // print_message("Connection found", GREEN);
                        // std::cout << "test2 " << it->second->fd << std::endl;
                        handleRequest(it->second);

                        break;
                    }
               
                }
                // std::cout << "last activety" << connections[j].find(current_fd)->second->last_active << std::endl;
  
            }
            

        }
            time_t current_time = time(0);
            for (size_t j = 0; j < servers.size(); j++) {
                std::vector<int> expired_fds;
                
                // First, collect all expired connections
                for (std::map<int, Connection*>::iterator it = this->connections[j].begin(); 
                    it != this->connections[j].end(); ++it) {
                    if (current_time - it->second->last_active > KEEP_ALIVE_TIMEOUT) {
                        expired_fds.push_back(it->first);
                        std::cout << "Found expired connection " << it->first 
                                << " (inactive for " << current_time - it->second->last_active 
                                << " seconds)" << std::endl;
                    }
                }
                
                // Then close them
                for (size_t i = 0; i < expired_fds.size(); ++i) {
                    std::map<int, Connection*>::iterator it = this->connections[j].find(expired_fds[i]);
                    if (it != this->connections[j].end()) {
                        std::cout << "Closing expired connection " << it->first << " on server " << j << std::endl;
                        Connection* conn = it->second;
                        close_connection(conn); // This will also erase from the map
                    }
                }
            }
        
    }
}

void Run::printrunservers()
{
    
    for (size_t i = 0; i < servers.size(); i++)
    {
        std::cout   <<servers[i]->getServerName() << " --> " 
                    << "http://" << servers[i]->getServerIp() << ":" 
                    << servers[i]->getPort() 
                    << " config index: " <<  servers[i]->getconnfig_index()
                    << std::endl;
    }
}
void Run::setnon_blocking(int fd)
{
    fcntl(fd, F_SETFL | O_NONBLOCK);
}

void Run::add_to_epoll(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}
void Run::mod_epoll(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}
void Run::remove_from_epoll(int fd)
{
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

// filepath: /home/mole_pc/Desktop/v4/runserver.cpp
Run::~Run()
{
    for (size_t i = 0; i < servers.size(); i++)
    {
        delete servers[i];
    }
    servers.clear();

    for (size_t i = 0; i < connections.size(); i++)
    {
        for (std::map<int, Connection *>::iterator it = connections[i].begin(); it != connections[i].end(); ++it)
        {
            delete it->second;
        }
        connections[i].clear();
    }
    configs.clear();
    std::cout << "Server closed" << std::endl;
    std::cout << "Bye bye" << std::endl;
}

void resetClient(Connection *conn)
{
    // Safely close the file if it's open
    if (conn->readFormFile && conn->readFormFile->is_open())
    {
        conn->readFormFile->close();
    }

    // Reset string buffers
    conn->read_buffer.clear();
    conn->write_buffer.clear();
    conn->path.clear();
    conn->query.clear();
    conn->upload_path.clear();
    conn->response.clear();

    // Reset state variables
    conn->method = NOTDETECTED;
    conn->last_active = time(0); // Set to current time instead of 0
    conn->content_length = 0;
    conn->total_sent = 0;
    conn->total_received = 0;
    conn->status_code = 200;

    // Reset connection flags
    conn->keep_alive = true;
    conn->headersSend = false;
    conn->chunked = false;
    conn->state = Connection::READING;
    // conn->validUpload = true;
    conn->is_cgi = false;
    
    // Reset file stream
    if (conn->readFormFile)
    {
        delete conn->readFormFile;
    }
    conn->readFormFile = new std::ifstream();

    std::cout << GREEN "Client connection reset" RESET << std::endl;
}

void Run::sendResponse(Connection *conn)
{
    if (conn->fd <= 0) {
        std::cerr << "No socket available" << std::endl;
        conn->state = Connection::CLOSING;
        return;
    }
    if (!conn->headersSend) {
        conn->write_buffer = conn->GetHeaderResponse();
        conn->headersSend = true;
        conn->total_sent = 0;
    }

    if (!conn->readFormFile->is_open() && !conn->is_cgi) {
        conn->state = Connection::CLOSING;
        return;
    }

    // Handle CGI response
    if (conn->is_cgi && !conn->response.empty()) {
        conn->write_buffer.append(conn->response);
        conn->content_length = conn->response.size(); // Set correct content length
        conn->response.clear(); // Clear to prevent resending
    } else {
        char buffer[BUFFER_SIZE];
        conn->readFormFile->read(buffer, BUFFER_SIZE);
        std::streamsize bytes_read = conn->readFormFile->gcount();
        conn->write_buffer.append(buffer, bytes_read);
    }

    ssize_t sent = send(conn->fd, conn->write_buffer.c_str(), conn->write_buffer.size(), MSG_NOSIGNAL);
    if (sent < 0) {
        conn->state = Connection::CLOSING;
        return;
    }

    conn->write_buffer.erase(0, sent);
    conn->total_sent += sent;

    // Check if all data is sent
    bool cgi_done = conn->is_cgi && conn->content_length <= conn->total_sent;
    bool file_done = !conn->is_cgi && conn->readFormFile->eof();

    if (cgi_done || file_done) {
        conn->readFormFile->close();
        conn->state = Connection::WRITING;
        if (conn->keep_alive == false)
        {
            std::cout << "close by keep alive   " << std::endl;
            conn->state = Connection::CLOSING;
            // close_connection(conn);
        }
        else
        {
            resetClient(conn);
            mod_epoll(conn->fd, EPOLLIN);
        }
    } else if (sent > 0) {
        mod_epoll(conn->fd, EPOLLOUT);
    }
}
void Run::close_connection(Connection *conn)
{
    if (!conn)
        return;
    
    int fd = conn->fd;
    int active = time(0) - conn->last_active;
    std::cout << "Connection closed after " << active << " seconds" << std::endl;
    // Set fd to an invalid value so other parts know it's been closed
    conn->fd = -1; 
    
    std::cout << "Closing connection: " << fd << std::endl;
    remove_from_epoll(fd);
    close(fd);
    
    // Remove from connections map BEFORE deletion
    for (size_t i = 0; i < servers.size(); i++)
    {
        this->connections[i].erase(fd);
    }
    
    delete conn;
}


// << =================== Methods for Server =================== >> //

int Run::GET_hander(Connection *conn, HTTPRequest &request)
{
    
    if (request.getPath() == "/favicon.ico")
    {
        conn->path = this->servers[this->currIndexServer]->getroot() + request.getPath();
    }
    else
        conn->path = request.getPath();
    
    // conn->GetStateFilePath();
    // conn->status_code = 200;

    return 0;
}

int Run::POST_hander(Connection *conn)
{
    if (!conn->response.empty())
    {
        conn->status_code = 200;
        return 0;
    }
    conn->status_code = 201;
    // conn->GetStateFilePath();
    return 0;
}
void deleteFile(std::string path)
{

    // Check if file exists before attempting to delete
    if (access(path.c_str(), F_OK) != 0)
    {
        std::cerr << "File does not exist: " << path << std::endl;
        return;
    }

    // Check if we have write permission to delete the file
    if (access(path.c_str(), W_OK) != 0)
    {
        std::cerr << "No permission to delete file: " << path << std::endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
        return;
    }

    // Attempt to delete the file
    if (remove(path.c_str()) != 0)
    {
        std::cerr << "Error deleting file: " << path << std::endl;
        std::cerr << "Error: " << strerror(errno) << std::endl;
    }
    else
    {
        std::cout << "File successfully deleted: " << path << std::endl;
    }
}

int Run::DELETE_hander(Connection *conn, HTTPRequest &request)
{

    conn->status_code = 204;
    deleteFile(request.getPath());
    // conn->GetStateFilePath();
    return 0;
}