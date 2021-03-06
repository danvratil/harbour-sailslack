/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include <QObject>
#include <QTest>

#include "lru.h"

class LRUTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testLRU()
    {
        LRU<int, 5> lru;
        QCOMPARE(lru.reference(1), std::optional<int>{});  // {1}
        QCOMPARE(lru.reference(2), std::optional<int>{});  // {2,1}
        QCOMPARE(lru.reference(3), std::optional<int>{});  // {3,2,1}
        QCOMPARE(lru.reference(4), std::optional<int>{});  // {4,3,2,1}
        QCOMPARE(lru.reference(5), std::optional<int>{});  // {5,4,3,2,1}
        QCOMPARE(lru.reference(6), std::optional<int>{1}); // {6,5,4,3,2}
        QCOMPARE(lru.reference(7), std::optional<int>{2}); // {7,6,5,4,3}
        QCOMPARE(lru.reference(3), std::optional<int>{});  // {3,7,6,5,4}
        QCOMPARE(lru.reference(8), std::optional<int>{4}); // {8,3,7,6,5}
        QCOMPARE(lru.reference(5), std::optional<int>{});  // {5,8,3,7,6}
        QCOMPARE(lru.reference(6), std::optional<int>{});  // {6,5,8,3,7}
        QCOMPARE(lru.reference(7), std::optional<int>{});  // {7,6,5,8,3}
        QCOMPARE(lru.reference(8), std::optional<int>{});  // {8,7,6,5,3}
        QCOMPARE(lru.reference(9), std::optional<int>{3}); // {9,8,7,6,5}
    }
};

QTEST_GUILESS_MAIN(LRUTest)

#include "tst_lru.moc"
