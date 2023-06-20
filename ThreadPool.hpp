#pragma once
#include "Thread.hpp"

#include "LockGuard.hpp"

#include <vector>
#include <queue>
#include <mutex>

#include <pthread.h>
#include <unistd.h>

using namespace std;

const int gnum = 3;

template<class T>
class ThreadPool;

template<class T>
class ThreadData {
public:
    ThreadPool <T> *threadpool;
    std::string name;
public:
    ThreadData(ThreadPool<T> *tp, const std::string &n) : threadpool(tp), name(n) {
    }
};

template<class T>
class ThreadPool {

private:
    static void* handlerTask(void *args) {
        ThreadData<T> *td = (ThreadData<T> *)args;
        while (true) {
            T t;
            {
                LockGuard lockguard(td -> threadpool -> mutex());
                while (td -> threadpool -> isQueueEmpty()) { // 不等于
                    td -> threadpool -> threadWait();
                }
                t = td -> threadpool -> pop();
            }
            cout << td -> name << "get a task : " << t.toTaskString() << "the end is : " << t() << endl;
        }
        delete td;
        return nullptr;
    }
    ThreadPool(const int &num = gnum) : _num(num) {
        pthread_mutex_init(&_mutex, nullptr);
        pthread_cond_init(&_cond,nullptr);
        for (int i = 0; i < _num; i++) {
            _threads.push_back(new Thread());
        }
    void operator=(const ThreadPool &) = delete;
    ThreadPool(const ThreadPool &) = delete;
    }
public:
    void lockQueue() { pthread_mutex_lock(&_mutex);}  // 上锁
    void unlockQueue() { pthread_mutex_unlock(&_mutex); } // 解锁
    bool isQueueEmpty() { return _task_queue.empty(); } // 阻塞队列是否为空
    void threadWait() { pthread_cond_wait(&_cond, &_mutex); } // 让对应的线程进行等待，等待被唤醒。即调用这个接口线程会被阻塞。
    T pop() {
        T t = _task_queue.front();
        _task_queue.pop();
        return t;
    }
    pthread_mutex_t *mutex() {
        return &_mutex;
    }
public:
    void run() {  
    // run() 函数的作用是遍历 _threads 容器中的线程对象，
    //并为每个线程创建一个 ThreadData 对象，然后启动线程，并在控制台输出线程启动的提示信息。
        for (const auto &t : _threads) {
            ThreadData<T> *td = new ThreadData<T>(this,t -> threadname());
            t -> start(handlerTask,td);
            cout << t -> threadname() << "start ..." << endl;
        }
    }
    void push(const T &in) {
        LockGuard lockguard(&_mutex); // 原子性过程 上锁！
        _task_queue.push(in);
        phtread_cond_signal(&_cond); // int pthread_cond_signal(pthread_cond_t *cond);  
        //唤醒一个在指定条件变量下等待的线程，一个一个唤醒时，所有线程以队列方式排列的
    }
    ~ThreadPool() {  // 析构函数！
        phtread_mutex_destory(&_mutex); // 销毁锁
        phtread_cond_destory(&_cond);  // 销毁条件变量！
        for (const auto &t : _threads) { 
            delete t;
        }
    }
    static ThreadPool<T> *getInstance() {
        if (nullptr == tp) {
            _singlock.lock();
            if (tp == nullptr) {
                tp = new ThreadPool<T>();
            }
            _singlock.unlock();
        }
        return tp;
    }
private:
    int _num;
    vector<Thread* > _threads; // 一组线程
    queue<T> _task_queue; // 阻塞队列
    phtread_mutex_t _mutex; // 互斥锁 —— 互斥！
    pthread_cond_t _cond; // 条件变量 —— 同步！

    static ThreadPool<T> *tp;
    static mutex _singlock;
};
template <class T>
ThreadPool<T> *ThreadPool<T>::tp = nullptr;

template <class T>
std::mutex ThreadPool<T>::_singlock;
