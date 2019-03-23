#ifndef SLACKAUTHENTICATOR_H
#define SLACKAUTHENTICATOR_H

#include <QObject>

class QNetworkAccessManager;

class SlackAuthenticator : public QObject
{
    Q_OBJECT

public:
    explicit SlackAuthenticator(QObject *parent = nullptr);
    ~SlackAuthenticator() override = default;

public Q_SLOTS:
    void fetchAccessToken(const QUrl &url);


private Q_SLOTS:
    void handleAccessTokenReply();

Q_SIGNALS:
    void accessTokenSuccess(const QString &userId, const QString &teamId, const QString &teamName, const QString &accessToken);
    void accessTokenFail();

private:
    QNetworkAccessManager *networkAccess = nullptr;
};

#endif // SLACKAUTHENTICATOR_H
