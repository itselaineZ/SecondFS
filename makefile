# CXX = g++
# CXXFLAGS = -I ./include/
# TARGET = SecondFS.exe
CC = c++
# CXX_FLAGS = -std=c++11 -w
# CXX_FLAGS = -std=gnu++11
SRC_DIR = src

BIN = SecondFS.exe

$(BIN): main.o Buffer.o BufferManager.o DeviceDriver.o File.o FileManager.o FileSystem.o INode.o OpenFileManager.o User.o Utility.o
	$(CC) -o $@ $^
main.o: $(SRC_DIR)/main.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
Buffer.o: $(SRC_DIR)/Buffer.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
BufferManager.o: $(SRC_DIR)/BufferManager.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
DeviceDriver.o: $(SRC_DIR)/DeviceDriver.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
File.o: $(SRC_DIR)/File.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
FileManager.o: $(SRC_DIR)/FileManager.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
FileSystem.o: $(SRC_DIR)/FileSystem.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
INode.o: $(SRC_DIR)/INode.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
OpenFileManager.o: $(SRC_DIR)/OpenFileManager.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
User.o: $(SRC_DIR)/User.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<
Utility.o: $(SRC_DIR)/Utility.cpp
	$(CC) $(CXX_FLAGS) -o $@ $<

.phony:
clean:
	rm -f *.o $(BIN)
	rm -f *.img