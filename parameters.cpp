#include "parameters.h"

std::string host::load(std::istream &s)
{
	is_ok_ = false;
	name_ = "localhost";
	root_ = "./";
	autoindex_ = false;
	
	std::string key;
	while (s >> key) {
		if (key == HOST_HOST) name_ = get_str(s);
		else if (key == HOST_ROOT) root_ = get_str(s);
		else if (key == HOST_AUTOINDEX) autoindex_ = get_bool(s);
		else return "Error: Unknown parameter: \"" + key + "\".";
	}
	
	if (name_.empty()) return "Error: Host name is empty.";
	if (root_.empty()) return "Error: Root path is empty.";
	
	struct stat st;
	if (lstat(root_.c_str(), &st) < 0) return "Error: Root path \"" + root_ + "\": " + std::string(strerror(errno)) + ".";
	if (S_ISLNK(st.st_mode)) return "Warning: Root path \"" + root_ + "\", that is symbolic link.";
	if (!S_ISDIR(st.st_mode)) return "Error: Root path \"" + root_ + "\" is not a directory.";
	
	is_ok_ = true;
	return "";
}

std::string parameters::load(const std::string &fpath)
{
	is_ok_ = false;
	port_ = max_clients_total_ = max_clients_thread_ = 0;
	hosts_.clear();
	
	std::ifstream fin(fpath.c_str());
	if (!fin) return "Error: Config file: \"" + fpath + "\": " + std::string(strerror(errno)) + '.';
	
	std::string key;
	while (fin >> key) {
		if (key == PAR_PORT) port_ = get_uint(fin);
		else if (key == PAR_MAX_CLIENTS_TOTAL) max_clients_total_ = get_uint(fin);
		else if (key == PAR_MAX_CLIENTS_THREAD) max_clients_thread_ = get_uint(fin);
		else if (key == PAR_THREADS) threads_ = get_uint(fin);
		else if (key == PAR_INCLUDE) {
			std::string host_path = get_str(fin);
			if (host_path.empty()) return "Error: Option \"include\" usage: \"include PATH\".";
			
			std::ifstream hin(host_path);
			if (!hin) return "Error: Config file: \"" + host_path + "\": " + std::string(strerror(errno)) + '.';
			host h;	// Инициализация хоста
			std::string tmp = h.load(hin);
			if (!h) return tmp;
			
			hosts_.insert(h);	// Добавление хоста
		} else return "Error: Unknown parameter: \"" + key + "\".";
	}
	
	if (max_clients_total_ == 0) return "Error: Invalid parameter: max_clients_total = 0.";
	if (max_clients_thread_ == 0) return "Error: Invalid parameter: max_clients_thread = 0.";
	if (hosts_.empty()) return "Error: No hosts configured.";
	
	is_ok_ = true;
	return "";
}
