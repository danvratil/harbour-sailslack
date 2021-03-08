TEMPLATE = subdirs

SUBDIRS += \
    tst_channellistmodel \
    tst_channelmessagesmodel \
    tst_channelsortmodel \
    tst_messagemodel \
    tst_lru \
    tst_threadmessagesmodel


OTHER_FILES += \
    *.xml

common.path = /opt/tests/harbour-sailslack/
common.files = tests.xml runtests.sh

INTSALLS += common
