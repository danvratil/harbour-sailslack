#ifndef UTILS_H
#define UTILS_H

#include <QtGlobal>

#include <type_traits>

#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)

template<typename T>
constexpr typename std::add_const<T>::type &qAsConst(T &v) noexcept { return v; }

template<typename T>
void qAsConst(T &&) = delete;

#endif

#ifndef Q_DISABLE_MOVE
#define Q_DISABLE_MOVE(Class) \
    Class(Class &&) noexcept = delete; \
    Class &operator=(Class &&) noexcept = delete;
#endif

#ifndef Q_DISABLE_COPY_MOVE
#define Q_DISABLE_COPY_MOVE(Class) \
    Q_DISABLE_COPY(Class) \
    Q_DISABLE_MOVE(Class)
#endif

#endif // UTILS_H
