#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <memory>
#include <mutex>

// Warning: If T's constructor throws, instance() will return a null reference.

namespace commonlibs
{

template<class T>
class Singleton
{
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

public:
    static T& instance()
    {
        std::call_once(flag_, init);
        return *t_;
    }

    static void init() // never throws
    {
        t_.reset(new T());
    }

protected:
    ~Singleton() {}
     Singleton() {}

private:
     static std::unique_ptr<T> t_;
     static std::once_flag flag_;

};

}

template<class T> std::unique_ptr<T> commonlibs::Singleton<T>::t_;
template<class T> std::once_flag commonlibs::Singleton<T>::flag_;

#endif
