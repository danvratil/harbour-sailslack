#ifndef SLACKCONFIG_H
#define SLACKCONFIG_H

#include <QObject>
#include <QSettings>

class SlackConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList teams READ getTeams NOTIFY teamsChanged)
public:
    explicit SlackConfig(QObject *parent = nullptr);
    ~SlackConfig() override = default;

    QStringList getTeams() const;

public Q_SLOTS:
    void addNewTeam();
    void removeTeam(const QString &team);

Q_SIGNALS:
    void teamsChanged();

    void teamAdded(const QString &team);
    void teamRemoved(const QString &team);

private:
    QSettings settings;
    QStringList teams;
};

#endif // SLACKCONFIG_H
