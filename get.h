#ifndef GET_H
#define GET_H

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>

#include <ctime>
#include <clocale>
#include <langinfo.h>

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#include "file_type.h"


class content
{
public:
	content(): is_ok_(false) {}
	virtual ~content() = 0;
	
	inline operator bool() const { return is_ok(); }
	inline bool is_ok() const { return is_ok_; }
	
	virtual int send(int s) = 0;
protected:
	bool is_ok_;
	std::string prefix_;
};


class dir_content: public content
{
public:
	dir_content(const std::string &dir_name, const std::string &host);
	
	int send(int s);
private:
	std::stringstream data_;
};


class file_content: public content
{
public:
	file_content(const std::string &file_name);
	
	int send(int s);
private:
	std::ifstream data_;
};


class error_content: public content
{
public:
	error_content(int status);
	
	int send(int s);
};


class get
{
public:
	get(const std::string &path, const std::string &host);
	~get();
	
	inline operator bool() const { return is_ok(); }
	inline bool is_ok() const { return is_ok_; }
	
	int send(int s);
private:
	bool is_ok_;
	content *data_;
};

#endif // GET_H
