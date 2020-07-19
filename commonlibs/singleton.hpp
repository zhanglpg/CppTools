#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <boost/utility.hpp>
#include <boost/thread/once.hpp>
#include <boost/scoped_ptr.hpp>

// Warning: If T's constructor throws, instance() will return a null reference.

namespace commonlibs
{

template<class T>
class Singleton : private boost::noncopyable
{

public:
    static T& instance()
    {
        boost::call_once(init, flag);
        return *t;
    }

    static void init() // never throws
    {
        t.reset(new T());
    }

protected:
    ~Singleton() {}
     Singleton() {}

private:
     static boost::scoped_ptr<T> t;
     static boost::once_flag flag;

};

}

template<class T> boost::scoped_ptr<T> commonlibs::Singleton<T>::t(0);
template<class T> boost::once_flag commonlibs::Singleton<T>::flag = BOOST_ONCE_INIT;

#endif
