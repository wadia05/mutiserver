#include "main.hpp"

void print_message(std::string message, std::string color)
{
	std::cout << color << message << RESET << std::endl;
}

int main(int ac, char** av) {

    if (ac != 2)
        return (print_message("Usage: ./webserv <config_file>", RED), 1);
    // MimeTypes mimeTypes("www/mimeTypes.csv");
    // std::ifstream file(av[1]);
    // Config config;
    // config.parseConfig(file);
    // configs = config.getConfigs();
    // // std::vector<Server *> servers;
    // for (size_t i = 0; i < configs.size(); i++) {
    //     for (size_t j = 0; j < configs[i].getPort().size(); j++) {
    //         Server *server = new Server( i, j);
    //         servers.push_back(server);
    //     }
    // }
    // for (size_t i = 0; i < servers.size(); i++) {
    //     delete servers[i];
    // }
    // exit(0);
    // exit(0);
    try {
        Run run(av);
        run.runServer();
        // Server server(config);
        // exit(1);
        // run();
    } catch (const std::exception& e) {
        
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
