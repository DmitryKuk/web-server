#ifndef SERVER_THREAD_H
#define SERVER_THREAD_H

#include <iostream>
#include <cstdio>

class socket_stream_handler
{
public:
	socket_stream_handler(int s): stream_(nullptr) { stream_ = fdopen(s, "r+"); }
	~socket_stream_handler() { if (stream_ != nullptr) fclose(stream_); }
	
	inline operator FILE *() const { return stream(); }
	inline FILE * stream() const { return stream_; }
private:
	FILE *stream_;
};


class socket_fd_handler
{
public:
	socket_fd_handler(int s): fd_(s) {}
	~socket_fd_handler() { close(fd_); }
	
	inline operator int() const { return fileno(); }
	inline int fileno() const { return fd_; }
private:
	int fd_;
};

#endif // SERVER_THREAD_H
