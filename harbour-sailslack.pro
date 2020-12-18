# App config
TARGET = harbour-sailslack
CONFIG += sailfishapp
SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

# Translations
CONFIG += sailfishapp_i18n
#TRANSLATIONS += translations/harbour-sailslack-fi.ts

# Notifications
QT += dbus
PKGCONFIG += nemonotifications-qt5

# Includes
INCLUDEPATH += ./QtWebsocket

QT += concurrent dbus

include(vendor/vendor.pri)

VERSION = "1.4.2"
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"

# Check slack config
CLIENT_ID = $$slack_client_id
CLIENT_SECRET = $$slack_client_secret
if(isEmpty(CLIENT_ID)) {
    error("No client id defined")
}

if(isEmpty(CLIENT_SECRET)) {
    error("No client secret defined")
}
DEFINES += SLACK_CLIENT_ID=\\\"$${CLIENT_ID}\\\"
DEFINES += SLACK_CLIENT_SECRET=\\\"$${CLIENT_SECRET}\\\"

DBUS_ADAPTORS += src/harbour.sailslack.xml

TRANSLATIONS += translations/harbour-sailslack.ts \
                translations/harbour-sailslack-sv.ts \
                translations/harbour-sailslack-bg.ts

SOURCES += src/harbour-sailslack.cpp \
    src/slackclient.cpp \
    src/QtWebsocket/QWsSocket.cpp \
    src/QtWebsocket/QWsFrame.cpp \
    src/QtWebsocket/functions.cpp \
    src/QtWebsocket/QWsHandshake.cpp \
    src/networkaccessmanagerfactory.cpp \
    src/networkaccessmanager.cpp \
    src/slackstream.cpp \
    src/storage.cpp \
    src/messageformatter.cpp \
    src/notificationlistener.cpp \
    src/filemodel.cpp \
    src/slackauthenticator.cpp \
    src/requestutils.cpp \
    src/slackclientconfig.cpp \
    src/slackconfig.cpp \
    src/teamsmodel.cpp

OTHER_FILES += qml/harbour-sailslack.qml \
    qml/cover/CoverPage.qml \
    rpm/harbour-sailslack.changes.in \
    rpm/harbour-sailslack.spec \
    rpm/harbour-sailslack.yaml \
    harbour-sailslack.desktop \
    harbour-sailslack.png

HEADERS += \
    src/slackclient.h \
    src/QtWebsocket/QWsSocket.h \
    src/QtWebsocket/QWsFrame.h \
    src/QtWebsocket/functions.h \
    src/QtWebsocket/QWsHandshake.h \
    src/networkaccessmanagerfactory.h \
    src/networkaccessmanager.h \
    src/slackstream.h \
    src/storage.h \
    src/messageformatter.h \
    src/notificationlistener.h \
    src/filemodel.h \
    src/slackauthenticator.h \
    src/requestutils.h \
    src/slackclientconfig.h \
    src/slackconfig.h \
    src/teamsmodel.h

DISTFILES += \
    qml/pages/LoginPage.qml \
    qml/pages/Loader.qml \
    qml/pages/ChannelList.qml \
    qml/pages/ChannelList.js \
    qml/pages/Channel.qml \
    qml/pages/MessageListItem.qml \
    qml/pages/MessageInput.qml \
    qml/pages/Image.qml \
    qml/pages/ConnectionPanel.qml \
    qml/pages/ChannelListView.qml \
    qml/pages/MessageListView.qml \
    qml/pages/About.qml \
    qml/pages/RichTextLabel.qml \
    qml/pages/AttachmentFieldGrid.qml \
    qml/pages/Attachment.qml \
    qml/pages/Spacer.qml \
    qml/pages/ChannelSelect.qml \
    qml/pages/Channel.js \
    qml/pages/GroupLeaveDialog.qml \
    qml/pages/ChatSelect.qml \
    qml/dialogs/ImagePicker.qml \
    qml/pages/FileSend.qml \
    data/emoji.json \
    qml/pages/TeamList.qml \
    qml/pages/TeamList.js \
    qml/pages/UserView.qml

RESOURCES += \
    data.qrc
