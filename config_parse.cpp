#include <string>
#include <utility>
#include <vector>

class Server {
private:
    int port;
    std::string servername;

    typedef struct {
        std::string url;
        std::string root;
    } url_to_path_t;

    /* since urls differ in priority, it is more logical to
     * divide them by these priorities into different vectors */
    std::vector<url_to_path_t> exact_match_urls;
    std::vector<url_to_path_t> preferential_prefix_urls;
    std::vector<url_to_path_t> regex_match_urls;
    std::vector<url_to_path_t> prefix_match_urls;
public:
    Server(int port, std::string servername) {
        this->port = port;
        this->servername = std::move(servername);
    }

    void add_exact_match_url(std::string url, std::string root) {
        exact_match_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
    }

    void add_preferential_prefix_urls(std::string url, std::string root) {
        preferential_prefix_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
    }

    void add_regex_match_urls(std::string url, std::string root) {
        regex_match_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
    }

    void add_prefix_match_urls(std::string url, std::string root) {
        prefix_match_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
    }

};

class Settings {
private:
    struct url_to_path {
        std::string url;
        std::string path;
    };

    int count_working_process;
    std::vector<class Server> servers;


    // parses the config file, initializing all the data structures in the class
    void parse_config(std::string filename);

public:
    Settings(std::string filename) {
        parse_config(filename);
    }

};
