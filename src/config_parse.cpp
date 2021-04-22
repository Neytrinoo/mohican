#include <string>
#include <utility>
#include <vector>
#include <fstream>
#include "config_parce.h"


ServerSettings::ServerSettings(int port, std::string servername) {
    this->port = port;
    this->servername = std::move(servername);
}

void ServerSettings::add_exact_match_url(std::string url, std::string root) {
    exact_match_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
}


void ServerSettings::add_preferential_prefix_urls(std::string url, std::string root) {
    preferential_prefix_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
}

void ServerSettings::add_regex_match_urls(std::string url, std::string root) {
    regex_match_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
}

void ServerSettings::add_prefix_match_urls(std::string url, std::string root) {
    prefix_match_urls.push_back(url_to_path_t{std::move(url), std::move(root)});
}


MainServerSettings::MainServerSettings(std::string config_file_name) : config_file_name(std::move(config_file_name)) {
    parse_config();
}

void MainServerSettings::parse_config() {
    std::ifstream config_file(this->config_file_name);



    config_file.close();
}
