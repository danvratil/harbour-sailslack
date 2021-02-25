# App config
TEMPLATE = app
TARGET = harbour-sailslack
CONFIG += link_pkgconfig
PKGCONFIG += qt5embedwidget

CONFIG += sailfishapp
SAILFISHAPP_ICONS = 86x86 108x108 128x128 256x256

# Translations
#TRANSLATIONS += translations/harbour-sailslack-fi.ts

PKGCONFIG += nemonotifications-qt5 connman-qt5

CONFIG += sailfish-components-webview-qt5

QT += concurrent dbus websockets

VERSION = "0.2"
DEFINES += APP_VERSION=\\\"$${VERSION}\\\"

include(../defaults.pri)
include(../vendor/vendor.pri)
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

DBUS_ADAPTORS += harbour.sailslack.xml

include(models/models.pri)

SOURCES += harbour-sailslack.cpp \
    authserver.cpp \
    slackclient.cpp \
    networkaccessmanagerfactory.cpp \
    networkaccessmanager.cpp \
    slackstream.cpp \
    storage.cpp \
    messageformatter.cpp \
    notificationlistener.cpp \
    filemodel.cpp \
    slackauthenticator.cpp \
    requestutils.cpp \
    slackclientconfig.cpp \
    slackconfig.cpp \
    teamsmodel.cpp

OTHER_FILES += \
    ../harbour-sailslack.png
    ../data/emoji.json

HEADERS += \
    authserver.h \
    slackclient.h \
    networkaccessmanagerfactory.h \
    networkaccessmanager.h \
    slackstream.h \
    storage.h \
    messageformatter.h \
    notificationlistener.h \
    filemodel.h \
    slackauthenticator.h \
    requestutils.h \
    slackclientconfig.h \
    slackconfig.h \
    teamsmodel.h \
    utils.h

DISTFILES += \
    qml/harbour-sailslack.qml \
    qml/pages/LoginPage.qml \
    qml/pages/Loader.qml \
    qml/pages/ChannelList.qml \
    qml/pages/ChannelList.js \
    qml/pages/Channel.qml \
    qml/pages/LoginWebView.qml \
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
    qml/pages/TeamList.qml \
    qml/pages/TeamList.js \
    qml/pages/Thread.qml \
    qml/pages/UserView.qml \
    qml/cover/CoverPage.qml

RESOURCES += \
    ../data.qrc
