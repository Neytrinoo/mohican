#pragma once

#include <string>
#include <vector>

#include "server_settings.h"

class MainServerSettings {
private:
    std::string config_file_name;
    int count_workflows;
    std::string access_log_file;
    std::string error_log_file;
    std::vector<ServerSettings> servers;

    bool is_access_log_file = false;
    bool is_error_log_file = false;
    bool is_any_server = false;

    enum numbers_of_properties {
        COUNT_WORKFLOWS_NUMBER = 1,
        ACCESS_LOG_NUMBER = 2,
        ERROR_LOG_NUMBER = 3
    };

public:
    const std::vector<std::string> valid_properties = {"http", "count_workflows", "access_log", "error_log", "server"};

    explicit MainServerSettings(std::string config_file_name);

    int get_number_of_properties(std::string property);

    void set_property(int number_of_property, std::string value);

    void add_server();

    ServerSettings &get_last_server();

    friend void parse_config(MainServerSettings &server);


};