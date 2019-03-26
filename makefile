# CC := g++
# CFLAGS := -Wall -g --std=c++11 -lpthread

# OBJECTS := twebserv.o socklib.o handle.o common.o cthreadpool.o mutex.o condvar.o
# TARGET := twebserv


# $(TARGET): $(OBJECTS)
# 	$(CC) $(CFLAGS) -o twebserv $(OBJECTS)
# twebserv.o: twebserv.cpp handle.h socklib.h
# 	$(CC) $(CFLAGS) -c $<
# socklib.o: socklib.cpp socklib.h
# 	$(CC) $(CFLAGS) -c $<
# handle.o: handle.cpp handle.h common.h
# 	$(CC) $(CFLAGS) -c $<
# common.o: common.cpp common.h handle.h
# 	$(CC) $(CFLAGS) -c $<
# cthreadpool.o: cthreadpool.cpp cthreadpool.h condvar.h mutex.h
# 	$(CC) $(CFLAGS) -c $<
# mutex.o: mutex.cpp mutex.h
# 	$(CC) $(CFLAGS) -c $<
# condvar.o: condvar.cpp condvar.h
# 	$(CC) $(CFLAGS) -c $<

# .PHONY:clean
# clean:
# 	-rm -f $(TARGET) *.o




#把所有的目录做成变量，方便修改和移植
DIR_BIN = ./bin
DIR_SRC_1 = ./
DIR_SRC_2 = ./threadpool
DIR_SRC_3 = ./myepoll
DIR_OBJ = ./obj

#提前所有源文件(即：*.c文件)和所有中间文件(即：*.o)
SRC = $(wildcard ${DIR_SRC_1}/*.cpp ${DIR_SRC_2}/*.cpp ${DIR_SRC_3}/*.cpp)
OBJ = $(patsubst %.cpp, ${DIR_OBJ}/%.o, $(notdir ${SRC}))
INC = $(wildcard ${DIR_SRC_1}/*.h ${DIR_SRC_2}/*.h)
#设置最后目标文件
TARGET = twebserv
BIN_TARGET = ${DIR_BIN}/${TARGET}

CC = g++
CFLAGS = -g -Wall -lpthread --std=c++11 

#用所有中间文件生成目的文件，规则中可以用 $^替换掉 ${OBJECT}
${BIN_TARGET}:${OBJ}
	$(CC) $(CFLAGS) ${OBJ} -o $@ 

#生成各个中间文件
${DIR_OBJ}/%.o:${DIR_SRC_3}/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
${DIR_OBJ}/%.o:${DIR_SRC_2}/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<
${DIR_OBJ}/%.o:${DIR_SRC_1}/%.cpp
	$(CC) $(CFLAGS) -o $@ -c $<


.PHONY:clean
clean:
	find $(DIR_OBJ) -name *.o -exec rm -rf {} \;
	rm -rf $(BIN_TARGET)