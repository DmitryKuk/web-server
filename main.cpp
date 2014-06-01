#include <iostream>
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <pthread.h>

#include "parameters.h"
#include "get.h"
#include "socket_handler.h"
#include "utils.h"

#define DEFAULT_CONF "./config/web-server.conf"

#define SPACE "    "	// Отступ для печати


class counter
{
public:
	counter(std::atomic_uint &i): count_(i) { ++count_; }
	~counter() { --count_; }
private:
	std::atomic_uint &count_;
};


struct process_client_arg
{
	std::list<std::pair<pthread_t, process_client_arg>> *threads;
	std::mutex *threads_mutex;
	parameters *par;
	
	std::atomic_uint &threads_count;
	int s;
	
	std::list<std::pair<pthread_t, process_client_arg>>::iterator self_it;
};

void * process_client(process_client_arg *arg)
{
	[&] {	// Начало запускаемой лямбды
		counter c(arg->threads_count);	// Счётчик количества запущенных потоков с автоинкрементом/декрементом
		socket_stream_handler s(arg->s);		// Держатель сокета с автозакрытием
		if (s == nullptr) {
			std::clog << "Error: Can't open socket as a stream: " << strerror(errno) << '.' << std::endl;
			close(arg->s);
			return;
		}
		setlinebuf(s);	// Включение построчной буферизации
		
		// Получение запроса
		std::vector<std::stringstream> request;
		{
			// Получение данных не дольше 20 секунд
			struct timeval tv = { .tv_sec = 20, .tv_usec = 0 };	// Время ожидания
			int s_fd = fileno(s);
			fd_set read_fds, error_fds;
			FD_ZERO(&read_fds); FD_ZERO(&error_fds);
			FD_SET(s_fd, &read_fds); FD_SET(s_fd, &error_fds);
			
			if (select(s_fd + 1, &read_fds, NULL, &error_fds, &tv) == 1 && FD_ISSET(s_fd, &read_fds)) {	// Ожидание данных
				char *line = NULL;
				size_t capacity = 0;
				ssize_t len;
				while ((len = getline(&line, &capacity, s)) > 2)	// Получаем строку, getline из cstdio
					request.push_back(std::stringstream(std::string(line, len - 2)));
				if (len < 0) return;
			}
			if (request.empty()) return;
		}
		
		// Получение параметров запроса
		std::string path, host_name;
		{
			std::string request_type, protocol_type;
			request[0] >> request_type >> path >> protocol_type;
			if (request_type != "GET" || protocol_type != "HTTP/1.1") {
				error_content(400).send(arg->s);
				std::clog << "Incorrect request: \"" << request_type << path << protocol_type << "\"." << std::endl;
				return;
			}
			
			// Ищем хост, игнорируем прочие заголовки
			for (auto &header: request) {
				std::string tmp;
				header >> tmp;
				if (header.eof()) continue;
				
				if (tmp == "Host:") {
					host_name = get_str_simple(header, false);	// getline из iostream
					break;
				}
			}
			if (host_name.empty()) return;
		}
		
		// Ищем запрошенный хост и формируем path
		{
			auto it = arg->par->hosts().find(host(host_name));
			if (it == arg->par->hosts().end()) {
				error_content(400).send(arg->s);
				std::clog << "Incorrect host: \"" << host_name << "\"; path: \"" << path << "\"." << std::endl;
				return;
			}
			
		}	
		
		// Выполняем запрос
		std::clog << "Processing: path: \"" << path << "\"; host name: \"" << host_name << "\"." << std::endl;
		get g(path, host_name);
		if (g.send(fileno(s))) std::clog << "Unable to send data: " << strerror(errno) << std::endl;
	}();	// Конец запускаемой лямбды
	
	// Удаляем запись о текущем потоке
	arg->threads_mutex->lock();
	arg->threads->erase(arg->self_it);
	arg->threads_mutex->unlock();
	pthread_exit(NULL);
}


int main(int argc, char **argv)
{
	// Получение параметров сервера
	parameters par;
	{
		std::string par_status;
		if (argc < 2) par_status = par.load(DEFAULT_CONF);
		else if (argc != 2) {
			std::cerr << "Incorrect arguments!" << std::endl << "Usage:" << std::endl << SPACE << argv[0] << " [CONF_FILE]" << std::endl;
			return 1;
		} else par_status = par.load(argv[1]);
		
		if (!par) {
			std::cerr << "Error: Can't load parameters:" << std::endl << SPACE << par_status << std::endl;
			return 2;
		}
		
		// Печать параметров
		std::clog << "Parameters:" << std::endl
				  << SPACE "Port:                    " << par.port() << std::endl
				  << SPACE "Max clients total:       " << par.max_clients_total() << std::endl
				  << SPACE "Max clients per thread:  " << par.max_clients_thread() << std::endl
				  << SPACE "Threads:                 " << par.threads() << std::endl
				  << SPACE "Hosts:" << std::endl;
		for (auto &host: par.hosts())	// Перечисление хостов
			std::clog << SPACE SPACE << host.name() << ':' << std::endl
					  << SPACE SPACE SPACE "Root:      \"" << host.root() << '\"' << std::endl
					  << SPACE SPACE SPACE "Autoindex: " << ((host.autoindex())? "on": "off") << std::endl;
	}
	
	
	// if (chroot(par.root_dir().c_str()) != 0) {
	// 	std::cerr << "Error: Can't chroot: " << strerror(errno) << '.' << std::endl;
	// 	return 30;
	// }
	
	
	// Создание сокета
	socket_fd_handler s = socket(PF_INET, SOCK_STREAM, getprotobyname("tcp")->p_proto);
	if (s < 0) {
		std::cerr << "Error: Can't create socket: " << strerror(errno) << '.' << std::endl;
		return 10;
	}
	
	
	{	// Привязка сокета к IP
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(par.port());
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		
		if (bind(s, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
			std::cerr << "Error: Can't bind the socket to the address: " << strerror(errno) << '.' << std::endl;
			return 20;
		}
	}
	
	// Ожидание подключений
	if (listen(s, 128) < 0) {
		std::cerr << "Server: Can't listen to socket: " << strerror(errno) << '.' << std::endl;
		return 30;
	}
	
	
	std::list<std::pair<pthread_t, process_client_arg>> threads;
	std::mutex threads_mutex;
	std::atomic_uint threads_count(0);
	while (true) {
		fd_set read_fds, error_fds;
		FD_ZERO(&read_fds);					FD_ZERO(&error_fds);
		FD_SET(STDIN_FILENO, &read_fds);	FD_SET(STDIN_FILENO, &error_fds);
		FD_SET(s, &read_fds);				FD_SET(s, &error_fds);
		int max_s = STDIN_FILENO;
		if (s > max_s) max_s = s;
		++max_s;
		
		// Ожидание запросов или ошибок
		int client_count;
		while ((client_count = select(max_s, &read_fds, nullptr, &error_fds, nullptr)) < 0 && errno == EINTR)
			;
		if (client_count <= 0) {	// Никто ничего не написал или произошла ошибка
			if (client_count < 0) std::cerr << "Error: Select error: " << strerror(errno) << '.' << std::endl;	// Ошибка
			break;
		}
		
		// Проверка на команду выйти
		if (FD_ISSET(STDIN_FILENO, &read_fds) || FD_ISSET(STDIN_FILENO, &error_fds)) {
			std::cin.get();
			if (std::cin.eof()) break;
		}
		
		// Ошибка на прослушиваемом сокете
		if (FD_ISSET(s, &error_fds)) {
			std::cerr << "Error: Listening socket error: " << strerror(errno) << '.' << std::endl;
			break;
		}
		
		// Кто-то подключился
		if (FD_ISSET(s, &read_fds)) {
			int client_s = accept(s, nullptr, nullptr);
			if (client_s < 0)
				std::cerr << "Server: Acceptor: Can't accept connection: " << strerror(errno) << '.' << std::endl;
			else {
				if (threads_count >= par.threads()) {
					error_content(500).send(client_s);
					close(client_s);
					std::cerr << "Internal server error. Threads: " << threads_count << std::endl;
					continue;
				}
				
				// Обработка клиента
				threads_mutex.lock();
				auto it = threads.insert(
					threads.end(),
					std::make_pair(
						pthread_t(),
						process_client_arg({
							.threads = &threads,
							.threads_mutex = &threads_mutex,
							.par = &par,
							.threads_count = threads_count,
							.s = client_s
						})
					)	// make_pair()
				);	// threads.insert()
				threads_mutex.unlock();
				it->second.self_it = it;
				pthread_create(&(it->first), NULL, (void * (*)(void *))process_client, &(it->second));
				std::clog << "Client accepted" << std::endl;
			}
		}
	}
	
	std::clog << "Shutting down..." << std::endl;
	for (auto &th: threads) pthread_cancel(th.first);
	std::clog << "Stopped." << std::endl;
	return 0;
}
