#pragma once

#include <string>

class UpstreamSettings {
private:
    std::string upstream_address;
    int weight = 1;
    int port = 80;

    bool is_ip;

    int is_sub_ip_num(std::string sub_ip_num);
public:
    explicit UpstreamSettings(std::string upstream_address, int weight = 1, int port = 80);

    UpstreamSettings &operator=(const UpstreamSettings &other) = default;

    UpstreamSettings(const UpstreamSettings &other) = default;

    UpstreamSettings() = default;

    void set_upstream_address(std::string upstream_address);

    void set_weight(int weight);

    std::string get_upstream_address();

    int get_weight();

    int get_port();

    bool is_ip_address();
};