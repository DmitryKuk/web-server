#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <unordered_set>
#include <cctype>

#include <sys/stat.h>
#include <errno.h>

#include "utils.h"


// Параметры для сервера
#define PAR_PORT				"port"	// Номер порта сервера
#define PAR_THREADS				"threads"	// Макс. число потоков
#define PAR_INCLUDE				"include"	// Включить хост с указанным конфигом

#define PAR_MAX_CLIENTS_TOTAL	"max_clients_total"		// Временно не поддерживается
#define PAR_MAX_CLIENTS_THREAD	"max_clients_thread"	// Временно не поддерживается


// Параметры для хоста
#define HOST_HOST				"host"	// Имя хоста
#define HOST_ROOT				"root"	// Корневая директория
#define HOST_AUTOINDEX			"autoindex"	// Включение автоиндексации для директорий


class host
{
public:
	host():
			is_ok_(false), autoindex_(false), name_("localhost"), root_("./")
	{}
	
	host(const std::string &name):
			is_ok_(false), autoindex_(false), name_(name), root_("./")
	{}
	
	host(std::istream &s):
			is_ok_(false), autoindex_(false), name_("localhost"), root_("./")
	{ load(s); }
	
	std::string load(std::istream &s);
	
	inline const std::string & name() const { return name_; }
	inline const std::string & root() const { return root_; }
	inline bool autoindex() const { return autoindex_; }
	
	inline bool is_ok() const { return is_ok_; }
	inline operator bool() const { return is_ok(); }
	
	inline bool operator ==(const host &other) const { return name() == other.name(); }
private:
	mutable bool is_ok_, autoindex_;
	std::string name_;
	mutable std::string root_;
};

struct host_hash: public std::hash<std::string>
{
	size_t operator()(const host &h) const { return std::hash<std::string>::operator()(h.name()); }
};


typedef std::unordered_set<host, host_hash> hosts_set;

class parameters
{
public:
	parameters():
			is_ok_(false), port_(80),
			max_clients_total_(100), max_clients_thread_(20)
	{}
	
	parameters(const std::string &fpath):
			is_ok_(false), port_(80),
			max_clients_total_(100), max_clients_thread_(20)
	{ load(fpath); }
	
	std::string load(const std::string &fpath);
	
	inline bool is_ok() const { return is_ok_; }
	inline operator bool() const { return is_ok(); }
	
	inline unsigned int port() const { return port_; }
	inline unsigned int max_clients_total() const { return max_clients_total_; }
	inline unsigned int max_clients_thread() const { return max_clients_thread_; }
	inline unsigned int threads() const { return threads_; }
	
	inline const hosts_set & hosts() const { return hosts_; }
private:
	bool is_ok_;
	
	unsigned int port_;
	unsigned int max_clients_total_, max_clients_thread_, threads_;
	
	hosts_set hosts_;
};

#endif // PARAMETERS_H
