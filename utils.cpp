#include "utils.h"

std::string get_str_simple(std::istream &s, bool can_skip_lines)
{
	std::string res;
	
	if (!s.good()) return res;
	
	char ch;
	if (can_skip_lines) s >> ch;
	else {
		while (isspace(ch = s.get())	// Пропуск пробелов
			&& s.good() && ch != '\n' && ch != '\v')
			;
	}
	
	if (ch != '\n' && ch != '\v') {	// Считывание строки до конца
		s.unget(); std::getline(s, res);
	}
	return res;
}

std::string get_str(std::istream &s, bool can_skip_lines)
{
	std::string res;
	
	if (!s.good()) return res;
	
	char ch;
	if (can_skip_lines) s >> ch;
	else {
		while (isspace(ch = s.get())	// Пропуск пробелов
			&& s.good() && ch != '\n' && ch != '\"' && ch != '\'' && ch != '\v')
			;
	}
	
	if (ch == '\"') {	// Считывание строки в двойных кавычках
		while ((ch = s.get()) != '\"' && s.good()) res += ch;
	} else if (ch == '\'') {	// Считывание строки в одинарных кавычках
		while ((ch = s.get()) != '\'' && s.good()) res += ch;
	} else if (ch != '\n') {	// Считывание строки до конца
		s.unget(); std::getline(s, res);
	}
	return res;
}

unsigned int get_uint(std::istream &s, bool can_skip_lines)
{
	unsigned int res = 0;
	
	if (!s.good()) return res;
	
	if (can_skip_lines) s >> res;
	else {
		std::string tmp_str = get_str(s, false);
		std::stringstream tmp_stream(tmp_str);
		tmp_stream >> res;
	}
	return res;
}

bool get_bool(std::istream &s, bool can_skip_lines)
{
	if (!s.good()) return false;
	
	std::string tmp = get_str(s, can_skip_lines);
	if (tmp == "on" || tmp == "true" || tmp == "1") return true;
	return false;
}
