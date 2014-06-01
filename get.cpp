#include "get.h"


static int send_string(int fd, const std::string &str)
{
	size_t need_send = str.size(), pos = 0;
	while (need_send > 0) {
		int status = write(fd, str.c_str() + pos, need_send);
		if (status <= 0) return 1;
		pos += status;
		need_send -= status;
	}
	return 0;
}

static int send_stream(int fd, std::istream &data)
{
	size_t size = 1000;
	char buf[size];
	while (true) {
		data.read(buf, size);
		size_t need_send = data.gcount(), pos = 0;
		
		if (need_send == 0) return 0;
		
		while (need_send > 0) {
			int status = write(fd, buf + pos, need_send);
			if (status <= 0) return 1;
			pos += status;
			need_send -= status;
		}
	}
	return 0;
}

static std::string get_server_name()
{
	static std::vector<std::string> names({
		"ZX_Spectrum/1997 (Sinclair_BASIC)",
		"c64/1986 (**** COMMODORE 64 BASIC V2 **** 64K RAM SYSTEM ****)",
		"thttpd/1.02 (Minix 2.0.2 i186)",
		"pear/6.2 (iOS 5.0.1)",
		"segasrv/1.0 (SEGA MEGA DRIVE HTTP SERVER ROM 1.0)",
		"Tea with milk (Full cup, with sugar)"
	});
	static bool need_srandom = true;
	
	if (need_srandom) {
		srandom(time(NULL));
		need_srandom = false;
	}
	
	return names[random() % names.size()];
}


content::~content() {}


dir_content::dir_content(const std::string &dir_name, const std::string &host)
{
	// Суффиксы размеров
	static char size_suff[] = "BKMGTP";	// Байты, килобайты, мегабайты, гигабайты, терабайты, петабайты
	
	
	if (dir_name.empty()) return;
	
	DIR *dir = opendir(dir_name.c_str());
	if (dir == nullptr) return;
	
	std::string path_beg(dir_name);
	if (*path_beg.rbegin() != '/') path_beg += '/';
	
	// Список содержимого
	std::list<std::vector<std::string>> content_list;
	
	struct dirent *dirp;
	while ((dirp = readdir(dir)) != nullptr) {
		if (std::string(".") == dirp->d_name || std::string("..") == dirp->d_name) continue;
		
		std::string path = path_beg + dirp->d_name;
		
		struct stat st;
		if (lstat(path.c_str(), &st) != 0) continue;
		
		std::string type;
		switch(st.st_mode & S_IFMT) {
		case S_IFDIR:
			type = "dir"; break;
		case S_IFLNK: case S_IFSOCK:
		case S_IFBLK: case S_IFCHR: case S_IFIFO:
			type = "other"; break;
		case S_IFREG: default:
			const char *p = strrchr(dirp->d_name, '.');
			if (p != nullptr) ++p;
			else p = dirp->d_name;
			type = file_type(p);
			break;	// Обычный файл
		}
		
		
		std::string size = "-";
		{	// Получаем размер
			std::stringstream buf;
			if (st.st_size < 1000) buf << st.st_size << size_suff[0];
			else {
				double tmp = st.st_size;
				unsigned int i;
				for (i = 0; tmp > 1000 && i < sizeof(size_suff); ++i)
					tmp /= 1024;
				buf << std::fixed << std::setprecision(2) << tmp << size_suff[i];
			}
			size = buf.str();
		}
		
		std::string date;
		{	// Форматируем время последнего изменения
			char datestring[256];
			struct tm *tm = localtime(&st.st_mtime);	// st_mtime - время последнего изменения
			strftime(datestring, sizeof(datestring), nl_langinfo(D_T_FMT), tm);
			date = datestring;
		}
		
		content_list.push_back(std::vector<std::string>({ std::string(dirp->d_name), date, size, type }));
	}
	closedir(dir);
	
	content_list.sort(
		[] (const std::vector<std::string> &a, const std::vector<std::string> &b) -> bool {
			return a[0] < b[0];
		});
	
	// Начало страницы
	data_ << "<html>"
			 "<head>"
			 "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">"
			 "<title>Index of " << dir_name << "</title>"
			 "<style>pre{font-family: monospace;}</style>"
			 "</head>\r\n"
		  << "<body bgcolor=\"white\">\r\n"
		  << "<h1>Index of " << dir_name << "</h1><hr><pre>\r\r";
	
	// Начало ссылки
	std::string href_beg = "<a href=\"";
	if (strncmp(host.c_str(), "http://", 7) != 0) href_beg += "http://";
	href_beg += host;
	if (*href_beg.rbegin() != '/') href_beg += '/';
	if (path_beg != "./") href_beg += path_beg;
	
	// Середина и конец ссылки
	std::string href_mid = "\">", href_end = "</a>";
	
	// Ссылка на уровень выше
	if (dir_name != "/")
		data_ << href_beg << "../" << href_mid << "../" << href_end << std::endl;
	
	for (auto &x: content_list) {
		std::string tmp = href_beg + x[0] + href_mid + x[0] + href_end;
		data_ << std::setw(100) << std::left << tmp << x[1] << "    " << std::setw(9) << std::right << x[2] << "    " << x[3] << std::endl;
	}
	
	data_ << "</pre><hr>"
			 "<div style=\"position: absolute !important;\"></div>"
			 "</body>"
			 "</html>\r\n\r\n";
	
	// Начало ответа
	{
		std::stringstream tmp;
		tmp << "HTTP/1.1 200 OK\r\n"
			<< "Server: " << get_server_name() << "\r\n"
			   "Content-Type: text/html; charset=utf-8\r\n"
			   "Content-Length: " << data_.str().size() << "\r\n\r\n";
		prefix_ = tmp.str();
	}
	is_ok_ = true;
}

int dir_content::send(int s)
{
	if (send_string(s, prefix_)
		|| send_stream(s, data_))
		return 1;
	return 0;
}


file_content::file_content(const std::string &file_name):
		data_(file_name.c_str())
{
	data_.seekg(0, std::ios_base::end);
	
	// Начало ответа
	{
		std::stringstream tmp;
		tmp << "HTTP/1.1 200 OK\r\n"
			<< "Server: " << get_server_name() << "\r\n"
			   "Content-Length: " << data_.tellg() << "\r\n\r\n";
		prefix_ = tmp.str();
	}
	data_.seekg(0, std::ios_base::beg);
	
	if (data_) is_ok_ = true;
}

int file_content::send(int s)
{
	if (send_string(s, prefix_)
		|| send_stream(s, data_))
		return 1;
	return 0;
}


error_content::error_content(int status)
{
	switch (status) {
	case 500:
		prefix_ = "HTTP/1.1 500 Internal Server Error\r\nServer: " + get_server_name() + "\r\n\r\n"; break;
	case 403:
		prefix_ = "HTTP/1.1 403 Forbidden\r\nServer: " + get_server_name() + "\r\n\r\n"; break;
	case 404:
		prefix_ = "HTTP/1.1 404 Not Found\r\nServer: " + get_server_name() + "\r\n\r\n"; break;
	case 400: default:
		prefix_ = "HTTP/1.1 400 Bad Request\r\nServer: " + get_server_name() + "\r\n\r\n"; break;
	}
	is_ok_ = true;
}

int error_content::send(int s)
{
	if (send_string(s, prefix_)) return 1;
	return 0;
}


get::get(const std::string &path, const std::string &host):
		data_(nullptr)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0) {
		switch (errno) {
		case EACCES:
			data_ = new error_content(403); break;	// Permission denied
		case EFAULT:
			case ENOENT: data_ = new error_content(404); break;	// Not found
		case ENOTDIR: default:
			data_ = new error_content(400); break;	// Bad request
		}
		return;
	} else if (S_ISDIR(st.st_mode)) data_ = new(std::nothrow) dir_content(path, host);
	else data_ = new(std::nothrow) file_content(path);
	
	if (data_ != nullptr) is_ok_ = true;
}

get::~get()
{
	if (data_ != nullptr) delete data_;
}

int get::send(int s)
{
	if (data_ == nullptr) return 2;
	return data_->send(s);
}
