/* SPDX-License-Identifier: GPL-3.0-only
 * SPDX-FileCopyrightText: 2021 Daniel Vrátil <me@dvratil.cz>
 */
#ifndef CHANNELMESSAGESMODEL_H
#define CHANNELMESSAGESMODEL_H

#include <QAbstractProxyModel>

class ChannelMessagesModel : public QAbstractProxyModel
{
    Q_OBJECT

    Q_PROPERTY(QString channelId READ channelId WRITE setChannelId NOTIFY channelIdChanged)
public:
    explicit ChannelMessagesModel(QObject *parent = nullptr);
    ~ChannelMessagesModel() override = default;

    void setChannelId(const QString &channelId);
    QString channelId() const;

    void setSourceModel(QAbstractItemModel *sourceModel) override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;

    QModelIndex mapToSource(const QModelIndex &proxyIndex) const override;
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const override;

Q_SIGNALS:
    void channelIdChanged(const QString &channelId);

private Q_SLOTS:
    void onRowsAboutToBeInserted(const QModelIndex &sourceParent, int first, int last);
    void onRowsInserted(const QModelIndex &sourceParent, int first, int last);
    void onRowsAboutToBeMoved(const QModelIndex &sourceParent, int first, int last, const QModelIndex &sourceDestination);
    void onRowsMoved(const QModelIndex &sourceParent, int first, int last, const QModelIndex &sourceDestination);
    void onRowsAboutToBeRemoved(const QModelIndex &sourceParent, int first, int last);
    void onRowsRemoved(const QModelIndex &sourceParent, int first, int last);
    void onDataChanged(const QModelIndex &sourceTopLeft, const QModelIndex &sourceBottomRight, const QVector<int> &roles);

private:
    void updateTopLevelIndex();

private:
    QString mChannelId;
    QPersistentModelIndex mSourceRootIndex;
};

#endif // CHANNELMESSAGESMODEL_H
