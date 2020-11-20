CXX       := g++
CXX_FLAGS := -std=c++17 -ggdb
CXX_FLAGS_RELEASE := -std=c++17 -O3

BIN     := .
SRC     := src
INCLUDE := include
INCLUDE2:= /home/admin/Desktop/influxdb-cxx/include

LIBRARIES   := /home/admin/Desktop/influxdb-cxx/build/lib/libInfluxDB.so
EXEDEBUG    := emeter-grabber-debug
EXERELEASE  := emeter-grabber-release


all: $(BIN)/$(EXEDEBUG) $(BIN)/$(EXERELEASE) 

run: clean all
	./$(BIN)/$(EXEDEBUG)

debug: $(BIN)/$(EXEDEBUG)

release: $(BIN)/$(EXERELEASE)

$(BIN)/$(EXEDEBUG): $(SRC)/*.cpp $(INCLUDE)/*.hpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) -I$(INCLUDE2) $^ -o $@ $(LIBRARIES)

$(BIN)/$(EXERELEASE): $(SRC)/*.cpp $(INCLUDE)/*.hpp
	$(CXX) $(CXX_FLAGS_RELEASE) -I$(INCLUDE) -I$(INCLUDE2) $^ -o $@ $(LIBRARIES)

clean:
	rm $(BIN)/$(EXEDEBUG)
	rm $(BIN)/$(EXERELEASE)
