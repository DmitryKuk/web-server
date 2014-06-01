# Makefile
# Author: Dmitry Kukovinets (d1021976@gmail.com)

# Исходники C
SRCS_CPP=main.cpp parameters.cpp get.cpp file_type.cpp utils.cpp


TARGET=web-server


# Компиляторы
GPP=g++ -Wall -std=c++0x

# Объектные файлы
OBJS=$(SRCS_CPP:.cpp=.o)


# Цели
.PHONY: all clear #install uninstall

all: $(TARGET)

clear:
	rm -f "$(TARGET)" $(OBJS)

# Конечная цель
$(TARGET): $(OBJS)
	$(GPP) -o $@ $^ -lpthread

# Неявные преобразования
%.o: %.cpp
	$(GPP) -o $@ -c $<
