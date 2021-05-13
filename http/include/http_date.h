#pragma once

#include <string>
#include <vector>

class Date {
 public:
    Date();
    std::string get_date() const;

 private:
    static std::string get_week_day(int week_day);
    static std::string add_leading_zero(int num);
    static std::string get_month(int month);

 private:
    std::string date_;

    static const std::vector<std::string> months_;
    static const std::vector<std::string> week_days_;
};
