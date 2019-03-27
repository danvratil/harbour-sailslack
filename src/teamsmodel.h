#ifndef TEAMSMODEL_H
#define TEAMSMODEL_H

#include <QAbstractListModel>

#include <memory>
#include <vector>

#include "slackclient.h"
#include "slackconfig.h"

/**
 * @note Normally I would've implemented this in QML, but due to QTBUG-50319 the
 * SlackClient instances were getting garbage-collected randomly, breaking the app.
 */
class TeamsModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(SlackConfig *slackConfig READ getSlackConfig WRITE setSlackConfig)
public:
    struct Team {
        Team(const QString &uuid, std::unique_ptr<SlackClient> client)
            : uuid(uuid), client(std::move(client)) {}

        QString uuid;
        std::unique_ptr<SlackClient> client;
    };


    enum Roles {
        UuidRole = Qt::UserRole + 1,
        ClientRole
    };

    TeamsModel(QObject *parent = nullptr);

    void setSlackConfig(SlackConfig *config);
    SlackConfig *getSlackConfig() const;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role) const override;

public Q_SLOTS:
    SlackClient *addTeam(const QString &userId, const QString &teamId, const QString &teamName, const QString &accessToken);
    void removeTeam(const QString &teamId);

    SlackClient *clientForTeam(const QString &teamId) const;

private:
    std::vector<Team> teams;
    SlackConfig *config = nullptr;
};

#endif // TEAMSMODEL_H
