#ifndef SLACKCONFIG_H
#define SLACKCONFIG_H

#include <QObject>
#include <QSettings>

class SlackConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString accessToken READ getAccessToken WRITE setAccessToken NOTIFY accessTokenChanged)
    Q_PROPERTY(QString userId READ getUserId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString teamId READ getTeamId WRITE setTeamId NOTIFY teamIdChanged)
    Q_PROPERTY(QString teamName READ getTeamName WRITE setTeamName NOTIFY teamNameChanged)
public:
    explicit SlackConfig(QObject *parent = 0);

    QString getAccessToken() const;
    void setAccessToken(QString accessToken);
    Q_INVOKABLE void clearAccessToken();

    QString getUserId() const;
    void setUserId(QString userId);

    QString getTeamId() const;
    void setTeamId(const QString &teamId);

    QString getTeamName() const;
    void setTeamName(const QString &teamName);

    static void clearWebViewCache();

signals:
    void accessTokenChanged(const QString &token);
    void userIdChanged(const QString &userId);
    void teamIdChanged(const QString &teamId);
    void teamNameChanged(const QString &teamName);

private:
    QSettings settings;
    QString accessToken;
    QString userId;
    QString teamId;
    QString teamName;
};

#endif // SLACKCONFIG_H
