#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <QAbstractListModel>
#include <QJsonArray>
#include <QJsonObject>

class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString currentChannelId READ currentChannelId WRITE setCurrentChannelId NOTIFY currentChannelIdChanged)

public:
    enum MessageRoles {
        IdRole = Qt::UserRole + 1,
        TextRole,
        UserIdRole,
        UserNameRole,
        TimestampRole,
        ThreadTsRole,
        ThreadCountRole,
        ReactionsRole,
        AttachmentsRole,
        FilesRole,
        IsEditedRole,
        IsOwnMessageRole,
        ChannelIdRole
    };

    explicit MessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString currentChannelId() const { return m_currentChannelId; }
    void setCurrentChannelId(const QString &channelId);

public slots:
    void updateMessages(const QJsonArray &messages);
    void addMessage(const QJsonObject &message);
    void updateMessage(const QJsonObject &message);
    void removeMessage(const QString &messageId);
    void clear();

signals:
    void currentChannelIdChanged();

private:
    struct Message {
        QString id;
        QString text;
        QString userId;
        QString userName;
        QString timestamp;
        QString threadTs;
        int threadCount;
        QJsonArray reactions;
        QJsonArray attachments;
        QJsonArray files;
        bool isEdited;
        bool isOwnMessage;
        QString channelId;
    };

    QList<Message> m_messages;
    QString m_currentChannelId;

    int findMessageIndex(const QString &timestamp) const;
    Message parseMessage(const QJsonObject &json) const;
};

#endif // MESSAGEMODEL_H
