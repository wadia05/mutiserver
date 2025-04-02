#pragma once

#include "main.hpp"

class Connection;  // Forward declaration




class Server {
    private :
        size_t connfig_index;
        int port;
        int server_fd;
        long max_upload_size;

        // int epoll_fd;
        // int is_ready;
        // size_t current_fd_index;

        std::string serverName;
        std::string serverIp;
        std::string root; // server root directory
        // std::string index_page; // html file  should be already uploded
        // std::string error_page;
        
        // std::map<int, Connection*> connections;
        // std::map<int, std::string> server_configs;
        // std::vector<int> ready_fds;
        // std::vector<Config> configs;
        // Config configss;
        


    public :

        // Server(const Config &config); // change parameter base on the parser
        Server(); // change parameter base on the parser
        ~Server();
        void setserverfd (int server_fd) { this->server_fd = server_fd; };
        void setPort (int port) { this->port = port; };
        void setServerName (std::string serverName){this->serverName = serverName;};
        void setServerIp (std::string serverIp){ this->serverIp = serverIp;};
        void setuploadSize (int max_upload_size){ this->max_upload_size = max_upload_size;};
        void setroot( std::string root){ this->root = root;};
        void setconnfig_index (size_t connfig_index) { this->connfig_index = connfig_index; };
        // void addConnection(int fd, Connection* conn) { this->connections[fd] = conn; };
        // void removeConnection(int fd) { this->connections.erase(fd); };
        // bool findConnection(int fd) { return this->connections.find(fd) != this->connections.end(); };
        size_t getconnfig_index () const { return this->connfig_index; };
        std::string getroot() const { return this->root; };
        int getPort () const { return this->port; };
        std::string getServerName () const { return this->serverName; };
        std::string getServerIp () const { return this->serverIp; };
        long getuploadSize () const { return this->max_upload_size; };
        int getserverfd () const { return this->server_fd; };
        

        // void set_non_blocking(int fd);
        // void add_to_epoll(int fd, uint32_t events);
        // void mod_epoll(int fd, uint32_t events);
        // void remove_from_epoll(int fd);
        // void handle_connection(int fd);
        // void handle_request(Connection* conn);
        // void parseReaquest(Connection* conn);
        // void possess_request(Connection* conn , HTTPRequest &request);
        // void read_request(Connection* conn);
        // void send_response(Connection* conn);
        // void close_connection(Connection* conn);
        // void reset_socket(Connection* conn);
        
        // //methods fonctions
        // int GET_hander(Connection *conn, HTTPRequest &request);
        // int POST_hander(Connection *conn);
        // int DELETE_hander(Connection *conn, HTTPRequest &request);

};