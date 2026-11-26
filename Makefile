CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror
SOURCES = main.cpp Configure/Configure.cpp parsing/FileReader.cpp SQL/SQL.cpp parsing/SQLparser.cpp
HEADERS = Configure/Configure.h SQL/SQL.h helper/ConsoleApp.hpp parsing/FileReader.h parsing/SQLparser.h struct/Vector.hpp

.PHONY: all clean

all: dbms

dbms: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o dbms

clean:
	rm -f dbms
