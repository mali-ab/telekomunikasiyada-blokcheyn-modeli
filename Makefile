CXX = g++
CXXFLAGS = -Wall -Iinclude -std=c++17
LIBS = -lssl -lcrypto -lpqxx -lpq -lboost_system -lpthread

SRC_DIR = src
OBJ_DIR = obj

SOURCES = main.cpp $(SRC_DIR)/blockchain.cpp $(SRC_DIR)/database.cpp \
          $(SRC_DIR)/signature.cpp $(SRC_DIR)/smartContract.cpp $(SRC_DIR)/p2p.cpp

OBJECTS = main.o $(OBJ_DIR)/blockchain.o $(OBJ_DIR)/database.o \
          $(OBJ_DIR)/signature.o $(OBJ_DIR)/smartContract.o $(OBJ_DIR)/p2p.o

TARGET = telekom_system

$(TARGET): $(OBJ_DIR) $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LIBS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) *.o $(TARGET)