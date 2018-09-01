CC 		= g++
CFLAGS	= -g -Wall --std=c++11 
CLIBS	= -l mysqlcppconn -l pthread -l event
TARGET	= ftpotd
FILES	= main.cpp virtfs.cpp db.cpp sockets.cpp

all: $(FILES)
	$(CC) $(CFLAGS) $(CLIBS) -o $(TARGET) $(FILES)

clean: $(TARGET)
	$(RM) $(TARGET)
