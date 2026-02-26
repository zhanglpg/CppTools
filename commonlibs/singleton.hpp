#pragma once
// Thread-safe singleton using C++11 magic statics (ISO 6.7 [stmt.dcl]).
// Replaces the previous implementation that used boost::call_once,
// boost::once_flag, and boost::scoped_ptr.

namespace commonlibs {

template<class T>
class Singleton {
public:
    static T& instance() {
        static T obj;   // guaranteed thread-safe initialisation in C++11
        return obj;
    }

    Singleton(const Singleton&)            = delete;
    Singleton& operator=(const Singleton&) = delete;

protected:
    Singleton()  = default;
    ~Singleton() = default;
};

} // namespace commonlibs
