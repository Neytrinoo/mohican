#pragma once

#include <string>

class UpstreamSettings {
private:
    std::string upstream_address;
    int weight = 1;
public:
    explicit UpstreamSettings(std::string upstream_address, int weight = 1);

    UpstreamSettings &operator=(const UpstreamSettings &other) = default;

    UpstreamSettings(const UpstreamSettings &other) = default;

    UpstreamSettings() = default;

    void set_upstream_address(std::string upstream_address);

    void set_weight(int weight);

    std::string get_upstream_address();

    int get_weight();
};