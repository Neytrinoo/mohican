#include <string>

class UpstreamSettings {
private:
    std::string upstream_address;
    int weight = 1;
public:
    UpstreamSettings(std::string upstream_address, int weight = 1);

    int set_upstream_address(std::string upstream_address);

    void set_weight(int weight);

    std::string get_upstream_address();

    int get_weight();
};