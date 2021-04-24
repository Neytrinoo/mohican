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

std::string ServerSettings::get_root(std::string url) {
    for (auto exact_match_url : exact_match_urls) {
        if (!exact_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(), 
                url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        if (url == exact_match_url.url) {
            return exact_match_url.root;
        }
    }

    for (auto preferential_match_url : preferential_prefix_urls) {
        if (!preferential_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(), 
                url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        if (url.find(preferential_match_url.url) == 0) {
            return preferential_match_url.root;
        }
    }

    for (auto regex_match_url : regex_match_urls) {
        if (!regex_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(), 
                url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        std::regex regex(regex_match_url.url);
        if (std::regex_match(url.cbegin(), url.cend(), regex)) {
            return regex_match_url.root;
        }
    }

    for (auto prefix_match_url : prefix_match_urls) {
        if (!prefix_match_url.case_sensitive) {
            std::transform(url.begin(), url.end(), 
                url.begin(), [](unsigned char c) -> unsigned char { return std::tolower(c); });
        }
        if (url.find(prefix_match_url.url) == 0) {
            return prefix_match_url.root;
        }
    }

    throw "404 exception";
}

MainServerSettings::MainServerSettings(std::string config_file_name) : config_file_name(std::move(config_file_name)) {
    parse_config();
}

void MainServerSettings::parse_config() {
    std::ifstream config_file(this->config_file_name);



    config_file.close();
}
