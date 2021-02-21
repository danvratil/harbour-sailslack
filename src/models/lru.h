/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#ifndef LRU_H
#define LRU_H

#include <deque>
#include <algorithm>
#include <optional>

#include "../utils.h"

//! LRU cache
/*! A cache of given size that automatically removes least recently referenced
 * elements when full. */
template<typename T, std::size_t Size>
class LRU
{
    static_assert(Size > 0);
public:
    //! Constructs a new, empty LRU.
    explicit LRU() = default;
    ~LRU() = default;

    //! References element \c val
    /*! If the element already exists in the LRU it moves it to the front of the cache.
     * If the element doesn't exist, it inserts it at the front of the cache. If, after
     * the insertion, the size of the cache is greater than \c Size, the element at the
     * and of the cache is evicted and returned.
     * \param[in] val value to move to the front of the cache
     * \return an optional element that has been evicted from the cache due to insertion, or
     * an empty value if no eviction happened. */
    std::optional<T> reference(const T &val)
    {
        auto it = std::find(mDeque.begin(), mDeque.end(), val);
        if (it == mDeque.end()) {
            return insertAndEvict(val);
        }

        moveToTop(it);
        return {};
    }

private:
    //! Prepends new element and evicts the last one if needed.
    /*! \param[in] val The element to prepend
     * \return the evicted elemented, if eviction occured. */
    std::optional<T> insertAndEvict(const T &val)
    {
        mDeque.push_front(val);
        if (mDeque.size() > Size) {
            const auto evicted = mDeque.back();
            mDeque.pop_back();
            return evicted;
        }

        return {};
    }

    //! Moves the element specified by the iterator to the front of the cache.
    /*! \param[in] it an iterator to the \c mDeque pointing to the element to be moved */
    void moveToTop(typename std::deque<T>::iterator it) {
        mDeque.push_front(std::move(*it));
        mDeque.erase(it);
    }

    std::deque<T> mDeque;

    Q_DISABLE_COPY_MOVE(LRU)
};

#endif // LRU_H
