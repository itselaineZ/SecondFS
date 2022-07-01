CXX ?= g++
# CXXFLAGS = -I ./include/
# TARGET = SecondFS.exe
# CC = c++
# CXX_FLAGS = -std=c++11 -w
# CXX_FLAGS = -std=gnu++11
SRC_DIR = src

DEBUG ?= 1
ifeq ($(DEBUG), 1)
	CXXFLAGS += -g
else
	CXXFLAGS += -O2

endif
    
all: $(SRC_DIR)/main.cpp $(SRC_DIR)/BufferManager.cpp $(SRC_DIR)/DiskDriver.cpp $(SRC_DIR)/File.cpp $(SRC_DIR)/FileManager.cpp $(SRC_DIR)/FileSystem.cpp $(SRC_DIR)/Inode.cpp $(SRC_DIR)/OpenfileManager.cpp $(SRC_DIR)/User.cpp $(SRC_DIR)/Utility.cpp
	$(CXX) -o secondFS  $^ $(CXXFLAGS)
    
.phony:
clean:
	rm -f secondFS
	rm -f *.img
