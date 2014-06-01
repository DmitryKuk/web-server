#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <sstream>
#include <string>

std::string get_str_simple(std::istream &s, bool can_skip_lines = false);
std::string get_str(std::istream &s, bool can_skip_lines = false);
unsigned int get_uint(std::istream &s, bool can_skip_lines = false);
bool get_bool(std::istream &s, bool can_skip_lines = false);

#endif	// UTILS_H
