#pragma once

#include <string>
#include <vector>

#include "upstream_settings.h"

typedef struct {
    std::string url;
    std::string root;
    std::string proxy_path;
    UpstreamSettings *upstream;
    bool case_sensitive;
    bool is_access_log;
    bool is_error_log;
    bool is_proxy;
} location_t;

class ServerSettings {
private:
    int port;
    std::string servername;
    std::string access_log_file;
    std::string error_log_file;
    std::string root;

    bool is_root = false;

    std::vector<location_t> exact_match_urls;
    std::vector<location_t> preferential_prefix_urls;
    std::vector<location_t> regex_match_urls;
    std::vector<location_t> prefix_match_urls;

    // the value of the elements enum is written the index of this key in the array of properties
    enum numbers_of_properties {
        LISTEN_NUMBER = 0,
        ACCESS_LOG_NUMBER = 1,
        ERROR_LOG_NUMBER = 2,
        ROOT_NUMBER = 3,
        SERVERNAME_NUMBER = 5
    };

    enum numbers_of_location_properties {
        ROOT_LOCATION_NUMBER = 0,
        ADD_ROOT_NUMBER = 1,
        ACCESS_LOG_LOCATION_NUMBER = 2,
        ERROR_LOG_LOCATION_NUMBER = 3,
        PROXY_PATH_LOCATION_NUMBER = 4
    };

    std::map<std::string, UpstreamSettings> upstreams;

    UpstreamSettings *get_upstream(std::string &upstream_address);
    /*
    const std::vector<std::string> valid_properties = {"listen", "root", "add_root", "access_log", "error_log",
                                                       "location"}; */
public:
    ServerSettings() = default;

    ServerSettings &operator=(const ServerSettings &other) = default;

    static const std::vector<std::string> valid_properties;

    static const std::vector<std::string> valid_location_properties;

    int get_number_of_property(std::string property);

    void set_property(int number_of_property, std::string value);

    void add_upstream(const std::string &upstream_address, int weight = 1);

    int get_number_of_location_property(std::string property);

    void set_location_property(int number_of_property, std::string value, location_t &location);

    void add_exact_match_url(location_t &location);

    void add_preferential_prefix_urls(location_t &location);

    void add_regex_match_urls(location_t &location);

    void add_prefix_match_urls(location_t &location);

    std::string get_access_log_filename();

    std::string get_error_log_filename();

    location_t get_location(std::string url);

    void print_properties();

    int get_port();

    std::string get_servername();

    friend int parse_location(ServerSettings &server, std::string &config, int &pos);
};
