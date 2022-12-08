#ifndef UTILITY_H_
#define UTILITY_H_

#include <vector>
#include <utility>
#include <string>
 
std::string trim(const std::string& s);

std::string join(std::vector<std::string>& vec, std::string delimiter);

std::vector<std::string> split(const std::string& s, const char& delimiter);

std::string remove_comment(const std::string& s);

std::vector<std::string> read_file(const std::string& file_name);
void write_file(const std::string& file_name, std::string timestamps);

int get_random(int lower_bound, int upper_bound);

#endif  // UTILITY_H_
