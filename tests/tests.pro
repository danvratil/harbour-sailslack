TEMPLATE = subdirs

SUBDIRS += \
    tst_messagemodel


OTHER_FILES += \
    *.xml

common.path = /opt/tests/harbour-sailslack/
common.files = tests.xml runtests.sh

INTSALLS += common
