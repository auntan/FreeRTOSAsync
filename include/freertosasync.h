#pragma once

#include <cassert>

#include "freertoshelpers.h"
#include "poolallocator.h"

namespace FreeRTOSAsync
{

// Maximum count of unresolved promises
constexpr int PROMISES_COUNT = 10;

class Future;
class Promise;
    
namespace Private
{
inline void then(Future& future, Promise& promise);
}

class Promise
{
public:
    Future getFuture();
    void setValue();

private:
    friend class Future;
    friend void Private::then(Future& future, Promise& promise);
    
private:
    Future *_future = nullptr;
    FreeRTOSHelpers::Function _then;
};

class Future
{
public:
    Future() = default;

    Future(Promise *promise) :
        _promise(promise)
    {
        _promise->_future = this;
    }

    bool deferred() const { return _promise != nullptr; }

    template <typename F>
    Future then(F f);

private:
    friend class Promise;
    friend void Private::then(Future& future, Promise& promise);
    
private:
    Promise *_promise = nullptr;
};

Future Promise::getFuture() { return Future(this); }

void Promise::setValue()
{
    if (_then) {
        _then();
    }
}

namespace Private
{
    
static PoolAllocator<Promise, PROMISES_COUNT>& pool()
{
    static PoolAllocator<Promise, PROMISES_COUNT> pool;
    return pool;
}

inline void then(Future& future, Promise& promise)
{
    if (future.deferred()) {
        future._promise->_then = [&promise]() {
            promise.setValue();
            Private::pool().free(&promise);
        };
    } else {
        promise.setValue();
        Private::pool().free(&promise);
    }
}

}

template <typename F>
Future Future::then(F f)
{ 
    Promise *newPromise = Private::pool().alloc();
    _promise->_then = [newPromise, f = std::move(f)]() {
        Future result = f();
        Private::then(result, *newPromise);
    };
    return newPromise->getFuture();
}

template <typename F>
Future setImmediate(F f)
{
    Promise *promise = Private::pool().alloc();
    Future future = promise->getFuture();

    FreeRTOSHelpers::setImmediate([promise, f = std::move(f)]() {
        Future result = f();
        Private::then(result, *promise);
    });

    return future;
}

template <typename F>
Future setTimeout(int interval, F f)
{
    Promise *promise = Private::pool().alloc();
    Future future = promise->getFuture();

    FreeRTOSHelpers::setTimeout(interval, [promise, f = std::move(f)]() {
        Future result = f();
        Private::then(result, *promise);
    });

    return future;
}

}