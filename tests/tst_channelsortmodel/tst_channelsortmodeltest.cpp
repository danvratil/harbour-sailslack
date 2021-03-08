/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include <QTest>
#include <QRegularExpression>

#include "modeltest.h"
#include "modelwatcher.h"
#include "channellistmodel.h"
#include "messagemodel.h"
#include "channelsortmodel.h"

#include <cmath>
#include <random>

bool operator<(const QVariantMap &l, const QVariantMap &r)
{
    static constexpr auto joinKeys = [](const QVariantMap &m) {
        return std::accumulate(m.keyBegin(), m.keyEnd(), QString{});
    };

    return joinKeys(l) < joinKeys(r);
}

class ChannelSortModelTest : public QObject
{
    Q_OBJECT

    static QString mapCategory(const QString &cat)
    {
        if (cat[0] == QLatin1Char('C')) {
            return QStringLiteral("channel");
        } else if (cat[0] == QLatin1Char('D')) {
            return QStringLiteral("chat");
        } else {
            return {};
        }
    }

    std::vector<QVariantMap> parseScenario(const QStringList &scenario) {
        std::vector<QVariantMap> rv;
        rv.reserve(scenario.size());

        const QRegularExpression expr("^(\\d) ([*CD]) (.*)$");
        Q_ASSERT(expr.isValid());
        for (const auto &s : scenario) {
            const auto match = expr.match(s);
            QVariantMap sc{
                {QStringLiteral("id"), match.captured(3)},
                {QStringLiteral("unread_count"), match.captured(1).toInt()},
                {QStringLiteral("category"), mapCategory(match.captured(2))},
                {QStringLiteral("name"), match.captured(3)}
            };
            if (match.captured(2) == QLatin1String("*")) {
                sc[QStringLiteral("is_starred")] = true;
            }

            rv.emplace_back(std::move(sc));
        }

        return rv;
    }

    void addScenario(const char *name, const QStringList &scenario) {
        int i = 0;
        QStringList perm = scenario;
        std::sort(perm.begin(), perm.end());
        std::mt19937 rand;
        for (int i = 0; i < std::min(40, static_cast<int>(tgamma(scenario.size() + 1))); ++i) {
            std::shuffle(perm.begin(), perm.end(), rand);
            const auto data = QStringLiteral("%1 (permutation %2").arg(name).arg(i);
            QTest::newRow(qUtf8Printable(data)) << scenario << perm;
        }
    }

private Q_SLOTS:

    void testSorting_data()
    {
        QTest::addColumn<QStringList>("scenario");
        QTest::addColumn<QStringList>("permutation");

        /* The syntax for scenario entry is
                unread_count: number of unread messages in the channel
                category: one of C (channel), D (direct message), * (starred)
                name: name of the channel

           The scenarios are sorted in the expected order. The test will then try to insert them
           in various pseudoranom permutations and verify that they are always sorted according to
           the original scenario.
        */

        addScenario("Sort by unread count within category", {
            QStringLiteral("7 * Channel 5"),
            QStringLiteral("5 * Channel 1")
        });

        addScenario("Sort categories", {
            QStringLiteral("0 * Starred chat"),
            QStringLiteral("0 C Channel"),
            QStringLiteral("0 D Direct message"),
        });

        addScenario("Sort by unread count within two categories", {
            QStringLiteral("9 C Channel 1"),
            QStringLiteral("4 C Channel 2"),
            QStringLiteral("7 D Msg 1"),
            QStringLiteral("4 D MSg 2")
        });

        addScenario("Sort by unread and name", {
            QStringLiteral("5 C Channel 3"),
            QStringLiteral("0 C Channel 1"),
            QStringLiteral("0 C Channel 2")
        });

        addScenario("Sort by unreach count, categories and name", {
            QStringLiteral("5 * Starred channel 2"),
            QStringLiteral("3 * Starred channel 1"),
            QStringLiteral("3 * Starred channel 3"),
            QStringLiteral("4 C Channel 3"),
            QStringLiteral("0 C Channel 1"),
            QStringLiteral("0 C Channel 2"),
            QStringLiteral("9 D DM 1"),
            QStringLiteral("5 D DM 2"),
            QStringLiteral("5 D DM 3")
        });

        addScenario("Sort locale-aware", {
            QStringLiteral("0 D Ája"),
            QStringLiteral("0 D Alena"),
            QStringLiteral("0 D Čeněk"),
            QStringLiteral("0 D Ctirad"),
            QStringLiteral("0 D Oliver"),
            QStringLiteral("0 D Ötzi"),
            QStringLiteral("0 D Tomáš")
        });
    }

    void testSorting()
    {
        QFETCH(QStringList, scenario);
        QFETCH(QStringList, permutation);

        MessageModel messageModel;
        ChannelListModel channelListModel;
        channelListModel.setSourceModel(&messageModel);
        ChannelSortModel sortModel;
        sortModel.setDynamicSortFilter(false);
        sortModel.setSourceModel(&channelListModel);

        const auto sc = parseScenario(scenario);
        const auto scPerm = parseScenario(permutation);

        std::for_each(scPerm.begin(), scPerm.end(), [&messageModel](const auto &s) { messageModel.addChannel(s); });
        QTest::qWait(1);
        sortModel.sort(0);
        QCOMPARE(sortModel.rowCount(), static_cast<int>(sc.size()));

        //sortModel.sort(0);
        for (int i = 0; i < sortModel.rowCount(); ++i) {
            const auto channel = sortModel.data(sortModel.index(i, 0), static_cast<int>(MessageModel::Role::Channel)).toMap();
            if (channel != sc[i]) {
                qDebug() << "model[" << i << "]:" << channel;
                qDebug() << "expec[" << i << "]:" << sc[i];
            }
            QCOMPARE(channel, sc[i]);
        }
    }

};

QTEST_GUILESS_MAIN(ChannelSortModelTest)

#include "tst_channelsortmodeltest.moc"
