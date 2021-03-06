/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#pragma once

#include <functional>

#include <QModelIndex>

enum class FilterBy {
    Index,
    Parent
};

template<typename Obj>
struct DefaultIndexMapper
{
    explicit DefaultIndexMapper(Obj *obj)
        : obj(obj)
    {}

    QModelIndex operator()(const QModelIndex &index) const
    {
        return obj->mapFromSource(index);
    }

    template<typename T>
    T operator()(T &&t) const
    {
        return std::forward<T>(t);
    }

private:
    Obj * const obj;
};

template<typename Obj, typename SlotPtr, typename Filter, typename IndexMapper = DefaultIndexMapper<Obj>>
struct SignalFilter
{

    SignalFilter(Obj *obj, SlotPtr slotPtr, Filter filter, FilterBy filterBy = FilterBy::Index)
        : obj(obj), slotPtr(slotPtr), filter(filter), filterBy(filterBy), indexMapper(obj)
    {}

    SignalFilter(Obj *obj, SlotPtr slotPtr, Filter filter, FilterBy filterBy, IndexMapper indexMapper)
        : obj(obj), slotPtr(slotPtr), filter(filter), filterBy(filterBy), indexMapper(indexMapper)
    {}

    template<typename ... Arg>
    void operator()(const QModelIndex &sourceIndex, Arg && ... arg)
    {
        if ((filterBy == FilterBy::Index && !filter(sourceIndex))
                || (filterBy == FilterBy::Parent && !filter(sourceIndex.parent()))) {
            return;
        }

        (obj->*slotPtr)(indexMapper(sourceIndex), indexMapper(arg) ...);
    }

private:
    Obj * const obj;
    const SlotPtr slotPtr;
    const Filter filter;
    const FilterBy filterBy;
    const IndexMapper indexMapper;
};

template<typename Obj, typename SlotPtr, typename Filter>
SignalFilter(Obj *, SlotPtr, Filter, FilterBy) -> SignalFilter<Obj, SlotPtr, Filter>;

template<typename Obj, typename SlotPtr, typename Filter, typename IndexMapper>
SignalFilter(Obj *, SlotPtr, Filter, FilterBy, IndexMapper) -> SignalFilter<Obj, SlotPtr, Filter, IndexMapper>;
