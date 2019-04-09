#pragma once

#include <cassert>

#include "freertoshelpers.h"

namespace FreeRTOSAsync
{

// Maximum count of unresolved promises
constexpr int PROMISES_COUNT = 100;

class Future;
class Promise;
    
namespace Private
{
inline void then(Future& future, Promise& promise);
}

class Promise
{
public:
    Promise() = default;

    Future getFuture();
    void setValue();

    bool busy = false;

    void setup()
    {
        new(this) Promise(true);
    }
    void free()
    {
        this->~Promise();
        busy = false;
    }

private:
    Promise(bool busy) :
        busy(true)
    {
    }

    friend class Future;

    friend void Private::then(Future& future, Promise& promise);
    
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
    Future then(F f)
    {        
	    Promise& _getPromise();
        Promise &newPromise = _getPromise();
        _promise->_then = [&newPromise, f = std::move(f)]() {
            Future result = f();
            Private::then(result, newPromise);
        };
        return newPromise.getFuture();
    }

private:
    friend class Promise;

    friend void Private::then(Future& future, Promise& promise);
    
    Promise *_promise = nullptr;
};

Future Promise::getFuture() { return Future(this); }

void Promise::setValue()
{
    if (_then) {
        _then();
    }
    free();
}

namespace Private
{

Promise& getPromise()
{
    static Promise promises[PROMISES_COUNT];
    
    for (int i = 0; i < PROMISES_COUNT; i++) {
        if (!promises[i].busy) {
            promises[i].setup();
            return promises[i];
        }
    }
    assert(false);
    return *(Promise*)nullptr;
}

inline void then(Future& future, Promise& promise)
{
    if (future.deferred()) {
        future._promise->_then = [&promise]() {
            promise.setValue();
        };
    } else {
        promise.setValue();
    }
}

}

template <typename F>
Future setImmediate(F f)
{
    Promise &promise = Private::getPromise();
    Future future = promise.getFuture();

    FreeRTOSHelpers::setImmediate([&promise, f = std::move(f)]() {
        Future result = f();
        Private::then(result, promise);
    });

    return future;
}

template <typename F>
Future setTimeout(int interval, F f)
{
    Promise &promise = Private::getPromise();
    Future future = promise.getFuture();

    setTimeout(interval, [&promise, f = std::move(f)]() {
        Future result = f();
        Private::then(result, promise);
    });

    return future;
}

}