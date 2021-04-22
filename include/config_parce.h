#pragma once

#include <string>
#include <algorithm>
#include <regex>

class ServerSettings {
private:
    int port;
    std::string servername;
    std::string access_log_filename;
    std::string error_log_filename;

    typedef struct {
        std::string url;
        std::string root;
        bool case_sensitive;
        bool is_access_log;
        bool is_error_log;
    } location_t;

    std::vector<location_t> exact_match_urls;
    std::vector<location_t> preferential_prefix_urls;
    std::vector<location_t> regex_match_urls;
    std::vector<location_t> prefix_match_urls;
public:
    ServerSettings(int port, std::string servername);

    void add_exact_match_url(std::string url, std::string root);

    void add_preferential_prefix_urls(std::string url, std::string root);

    void add_regex_match_urls(std::string url, std::string root);

    void add_prefix_match_urls(std::string url, std::string root);

    // the method should get a variable that points to the beginning of the "server" block
    void parse_config(std::ifstream config_file);

    std::string get_root(std::string url);
};

class MainServerSettings {
private:
    std::string config_file_name;
    int count_workflows;
    std::vector<class ServerSettings> servers;
    enum states {
            S_START,
            S_BRACE,
            S_KEY,
            S_VALUE,
            S_SERVER,
            S_ERR,
            S_COUNT,
            S_END
    };

    enum lexems {
        L_PROTOCOL,
        L_BRACE,
        L_NEW_LINE,
        L_KEY,
        L_VALUE,
        L_SERVER_START,
        L_SERVER_END,
        L_ERR,
        L_COUNT
    };

    void parse_config();

public:
    explicit MainServerSettings(std::string config_file_name);


};