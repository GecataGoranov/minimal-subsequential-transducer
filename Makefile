CXX ?= g++
CXXFLAGS ?= -std=c++20 -O2 -DNDEBUG -Wall -Wextra -pedantic
DEBUGFLAGS ?= -std=c++20 -O0 -g -D_GLIBCXX_ASSERTIONS -Wall -Wextra -pedantic

LIB_SOURCES = transducer.cpp real_time_transducer.cpp subsequential_transducer.cpp new_automaton.cpp
MAIN_SOURCES = $(LIB_SOURCES) main.cpp
TEST_SOURCES = $(LIB_SOURCES) tests/smoke_tests.cpp

TARGET = minimal_subsequential_transducer
TEST_TARGET = smoke_tests

.PHONY: all debug test clean

all: $(TARGET)

$(TARGET): $(MAIN_SOURCES)
	$(CXX) $(CXXFLAGS) $(MAIN_SOURCES) -o $(TARGET)

debug: $(MAIN_SOURCES)
	$(CXX) $(DEBUGFLAGS) $(MAIN_SOURCES) -o $(TARGET)_debug

test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCES)
	$(CXX) $(CXXFLAGS) $(TEST_SOURCES) -o $(TEST_TARGET)

clean:
	rm -f $(TARGET) $(TARGET)_debug $(TEST_TARGET) *.o

