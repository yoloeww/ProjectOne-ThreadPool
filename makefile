ThreadPool:main.cpp
	g++ -o $@ $^ -std=c++11 -lpthread
.PHONY:clean
clean:
	rm -f ThreadPool
