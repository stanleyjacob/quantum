/*
** Copyright 2018 Bloomberg Finance L.P.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/
//NOTE: DO NOT INCLUDE DIRECTLY

//##############################################################################################
//#################################### IMPLEMENTATIONS #########################################
//##############################################################################################
#include <quantum/quantum_future_state.h>
#include <quantum/quantum_local.h>

namespace Bloomberg {
namespace quantum {

#if (__cplusplus >= 201703L)

// Visitor helpers
template<typename... Ts>
struct Overloaded : Ts...
{
    using Ts::operator()...;
};

template<typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

template <typename T>
GenericFuture<T>::GenericFuture()
{}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadContextPtr<T> f) :
    _context(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadFuturePtr<T> f) :
    _context(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroContextPtr<T> f, ICoroSyncPtr sync) :
    _context(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroFuturePtr<T> f, ICoroSyncPtr sync) :
    _context(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(GenericFuture&& other) :
    _context(std::move(other._context)),
    _sync(std::move(other._sync))
{}

template <typename T>
GenericFuture<T>& GenericFuture<T>::operator=(GenericFuture&& other)
{
    if (this == &other) {
        return *this;
    }
    _context = std::move(other._context);
    _sync = std::move(other._sync);
    return *this;
}

template <typename T>
GenericFuture<T>::~GenericFuture()
{}

template <typename T>
bool GenericFuture<T>::valid() const
{
    return std::visit([](const auto& ctx)->bool { return ctx->valid(); }, _context);
}

template <typename T>
void GenericFuture<T>::wait() const
{
    std::visit(Overloaded {
        [](const ThreadContextPtr<T>& ctx) { assert(!local::context()); ctx->wait(); },
        [](const ThreadFuturePtr<T>& ctx) { assert(!local::context()); ctx->wait(); },
        [this](const CoroContextPtr<T>& ctx) { ctx->wait(_sync); },
        [this](const CoroFuturePtr<T>& ctx) { ctx->wait(_sync); }
    }, _context);
}

template <typename T>
std::future_status GenericFuture<T>::waitFor(std::chrono::milliseconds timeMs) const
{
    return std::visit(Overloaded {
        [&](const ThreadContextPtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->waitFor(timeMs); },
        [&](const ThreadFuturePtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->waitFor(timeMs); },
        [&, this](const CoroContextPtr<T>& ctx)->decltype(auto) { return ctx->waitFor(_sync, timeMs); },
        [&, this](const CoroFuturePtr<T>& ctx)->decltype(auto) { return ctx->waitFor(_sync, timeMs); }
    }, _context);
}

template <typename T>
template <typename V>
NonBufferRetType<V> GenericFuture<T>::get()
{
    return std::visit(Overloaded {
        [](const ThreadContextPtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->get(); },
        [](const ThreadFuturePtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->get(); },
        [this](const CoroContextPtr<T>& ctx)->decltype(auto) { return ctx->get(_sync); },
        [this](const CoroFuturePtr<T>& ctx)->decltype(auto) { return ctx->get(_sync); }
    }, _context);
}

template <typename T>
template <typename V>
const NonBufferRetType<V>& GenericFuture<T>::getRef() const
{
    return std::visit(Overloaded {
        [](const ThreadContextPtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->getRef(); },
        [](const ThreadFuturePtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->getRef(); },
        [this](const CoroContextPtr<T>& ctx)->decltype(auto) { return ctx->getRef(_sync); },
        [this](const CoroFuturePtr<T>& ctx)->decltype(auto) { return ctx->getRef(_sync); }
    }, _context);
}

template <typename T>
template <typename V>
BufferRetType<V> GenericFuture<T>::pull(bool& isBufferClosed)
{
    return std::visit(Overloaded {
        [&](const ThreadContextPtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->pull(); },
        [&](const ThreadFuturePtr<T>& ctx)->decltype(auto) { assert(!local::context()); return ctx->pull(); },
        [&, this](const CoroContextPtr<T>& ctx)->decltype(auto) { return ctx->pull(_sync, isBufferClosed); },
        [&, this](const CoroFuturePtr<T>& ctx)->decltype(auto) { return ctx->pull(_sync, isBufferClosed); }
    }, _context);
}

#else

template <typename T>
GenericFuture<T>::GenericFuture() :
    _type(Type::Invalid),
    _threadContext()
{}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadContextPtr<T> f) :
    _type(Type::ThreadContext),
    _threadContext(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(ThreadFuturePtr<T> f) :
    _type(Type::ThreadFuture),
    _threadFuture(f)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroContextPtr<T> f, ICoroSyncPtr sync) :
    _type(Type::CoroContext),
    _coroContext(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(CoroFuturePtr<T> f, ICoroSyncPtr sync) :
    _type(Type::CoroFuture),
    _coroFuture(f),
    _sync(sync)
{
    if (!f) {
        throw std::invalid_argument("Future pointer is null");
    }
    if (!_sync) {
        throw std::invalid_argument("Sync context is null");
    }
}

template <typename T>
GenericFuture<T>::GenericFuture(GenericFuture&& other) :
    _type(other._type),
    _threadContext(std::move(other._threadContext)), //any union member is ok
    _sync(std::move(other._sync))
{}

template <typename T>
GenericFuture<T>& GenericFuture<T>::operator=(GenericFuture&& other)
{
    if (this == &other) {
        return *this;
    }
    _type = other._type;
    _threadContext = std::move(other._threadContext); //any union member is ok
    _sync = std::move(other._sync);
    return *this;
}

template <typename T>
GenericFuture<T>::~GenericFuture()
{
    switch (_type) {
        case Type::ThreadContext:
            _threadContext.reset();
            break;
        case Type::ThreadFuture:
            _threadFuture.reset();
            break;
        case Type::CoroContext:
            _coroContext.reset();
            break;
        case Type::CoroFuture:
            _coroFuture.reset();
            break;
        default:
            break;
    }
}

template <typename T>
bool GenericFuture<T>::valid() const
{
    switch (_type) {
        case Type::ThreadContext:
            return _threadContext->valid();
        case Type::ThreadFuture:
            return _threadFuture->valid();
        case Type::CoroContext:
            return _coroContext->valid();
        case Type::CoroFuture:
            return _coroFuture->valid();
        default:
            return false;
    }
}

template <typename T>
void GenericFuture<T>::wait() const
{
    switch (_type) {
        case Type::ThreadContext:
            //This will block the coroutine
            assert(!local::context());
            _threadContext->wait();
            break;
        case Type::ThreadFuture:
            //This will block the coroutine
            assert(!local::context());
            _threadFuture->wait();
            break;
        case Type::CoroContext:
            _coroContext->wait(_sync);
            break;
        case Type::CoroFuture:
            _coroFuture->wait(_sync);
            break;
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
std::future_status GenericFuture<T>::waitFor(std::chrono::milliseconds timeMs) const
{
    switch (_type) {
        case Type::ThreadContext:
            //This will block the coroutine
            assert(!local::context());
            return _threadContext->waitFor(timeMs);
        case Type::ThreadFuture:
            //This will block the coroutine
            assert(!local::context());
            return _threadFuture->waitFor(timeMs);
        case Type::CoroContext:
            return _coroContext->waitFor(_sync, timeMs);
        case Type::CoroFuture:
            return _coroFuture->waitFor(_sync, timeMs);
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
template <typename V>
NonBufferRetType<V> GenericFuture<T>::get()
{
    switch (_type) {
        case Type::ThreadContext:
            //This will block the coroutine
            assert(!local::context());
            return _threadContext->get();
        case Type::ThreadFuture:
            //This will block the coroutine
            assert(!local::context());
            return _threadFuture->get();
        case Type::CoroContext:
            return _coroContext->get(_sync);
        case Type::CoroFuture:
            return _coroFuture->get(_sync);
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
template <typename V>
const NonBufferRetType<V>& GenericFuture<T>::getRef() const
{
    switch (_type) {
        case Type::ThreadContext:
            //This will block the coroutine
            assert(!local::context());
            return _threadContext->getRef();
        case Type::ThreadFuture:
            //This will block the coroutine
            assert(!local::context());
            return _threadFuture->getRef();
        case Type::CoroContext:
            return _coroContext->getRef(_sync);
        case Type::CoroFuture:
            return _coroFuture->getRef(_sync);
        default:
            throw FutureException(FutureState::NoState);
    }
}

template <typename T>
template <typename V>
BufferRetType<V> GenericFuture<T>::pull(bool& isBufferClosed)
{
    switch (_type) {
        case Type::ThreadContext:
            //This will block the coroutine
            assert(!local::context());
            return _threadContext->pull(isBufferClosed);
        case Type::ThreadFuture:
            //This will block the coroutine
            assert(!local::context());
            return _threadFuture->pull(isBufferClosed);
        case Type::CoroContext:
            return _coroContext->pull(_sync, isBufferClosed);
        case Type::CoroFuture:
            return _coroFuture->pull(_sync, isBufferClosed);
        default:
            throw FutureException(FutureState::NoState);
    }
}

#endif //(__cplusplus >= 201703L)
    
}}
