/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */

#include "messagemodel.h"
#include "modeltest.h"

class MessageModelTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testChannelPopulation()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
    }

    void testAddChannel()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.addChannel(QStringLiteral("channel-11"), QStringLiteral("Channel 11"));
    }

    void testUpdateChannel()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.updateChannel(4, QStringLiteral("Happy Chanel"));
    }

    void testSetChannelMessages()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(5, 20);
    }

    void testAppendChannelMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(3, 10);
        test.appendChannelMessage(3, QStringLiteral("100"), QStringLiteral("Hello there!"));
    }

    void testRemoveChannelMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(6, 5);
        test.removeChannelMessage(6, 1);
    }

    void testUpdateChannelMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(7, 10);
        test.updateChannelMessage(7, 4, QStringLiteral("FooBar"));
    }

    void testSetThreadMessages()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(3, 6);
        test.populateThreadMessages(3, 5, 10);
    }

    void testAppendChannelMessageStartsNewThread()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(7, 15);
        test.appendThreadMessage(7, 3, QStringLiteral("Let the thread begin"));
    }

    void testAppendChannelMessageToExistingThread()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(1, 5);
        test.populateThreadMessages(1, 2, 3);
        test.appendThreadMessage(1, 2, QStringLiteral("New message in thread"));
    }

    void testUpdateChannelMessageInThread()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(9, 10);
        test.populateThreadMessages(9, 9, 10);
        test.updateThreadMessage(9, 9, 5, QStringLiteral("Updated thread message"));
    }

    void testRemoveThreadMessage()
    {
        MessageModel model;
        ModelTest test{&model};

        test.populateChannels(10);
        test.populateChannelMessages(5, 10);
        test.populateThreadMessages(5, 5, 10);
        test.removeThreadMessage(5, 5, 5);
    }
};

QTEST_GUILESS_MAIN(MessageModelTest)

#include "tst_messagemodel.moc"
