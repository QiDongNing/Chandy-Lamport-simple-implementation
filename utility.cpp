#include "utility.h"

#include <functional>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <vector>
#include <sstream>
#include <regex>
#include <fstream>
#include <random>

using namespace std;

//函数移除字符串两侧的空白字符或其他预定义字符。功能除去字符串开头和末尾的空格或其他字符
std::string trim(const std::string &s)
{
	auto start = s.begin();
	while (start != s.end() && std::isspace(*start)) {
		start++;
	}

	auto end = s.end();
	do {
		end--;
	} while (std::distance(start, end) > 0 && std::isspace(*end));

	return std::string(start, end + 1);
}

/*
string join(vector<int>& vec, string delimiter) {
    ostringstream result_string;
    copy(vec.begin(), vec.end()-1, ostream_iterator<string>(result_string, delimiter.c_str()));
    result_string << vec.back();
    return result_string.str();
}
*/

vector<string> split(const string& s, const char& delimiter) {
    vector<string> result;
    stringstream sstream(s);
    string token;
    while (getline(sstream, token, delimiter)) {
        token = trim(token);
        result.push_back(token);
    }
    return result;
}

string remove_comment(const string& s) {
    string result;
    if (s.find('#') != string::npos) {
        result = split(s, '#')[0];
        return result;
    }
    return s;
}

vector<string> read_file(const string& file_name) {
    ifstream file(file_name);
    if (!file.is_open()) {
        printf("file error");
        exit(1);
    }

    string line;
    vector<string> lines;
    while (getline(file, line)) {
        if (line.length() == 0 || line.at(0) == '#')
            continue;
        line = remove_comment(line);
        lines.push_back(line);
    }

    file.close();
    return lines;
}

void write_file(const string& file_name, std::string timestamps) {
    ofstream file(file_name, ios::app);
    if (!file.is_open()) {
        printf("file error");
        exit(1);
    }
    file << timestamps;
    file.close();
}

int get_random(int lower_bound, int upper_bound) {
    std::random_device dev;
    std::mt19937 rng(dev());//伪随机数产生器
    std::uniform_int_distribution<std::mt19937::result_type> num(
        lower_bound, upper_bound); 
    return (int)num(rng);
}