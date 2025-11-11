#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>

struct Workspace {
    QString id;
    QString name;
    QString token;
    QString teamId;
    QString userId;
    QString domain;
    bool isActive;
    QDateTime lastUsed;
};

class WorkspaceManager : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int currentWorkspaceIndex READ currentWorkspaceIndex WRITE setCurrentWorkspaceIndex NOTIFY currentWorkspaceChanged)
    Q_PROPERTY(QString currentWorkspaceName READ currentWorkspaceName NOTIFY currentWorkspaceChanged)
    Q_PROPERTY(QString currentWorkspaceToken READ currentWorkspaceToken NOTIFY currentWorkspaceChanged)

public:
    enum WorkspaceRoles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TokenRole,
        TeamIdRole,
        UserIdRole,
        DomainRole,
        IsActiveRole,
        LastUsedRole
    };

    explicit WorkspaceManager(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int currentWorkspaceIndex() const { return m_currentWorkspaceIndex; }
    void setCurrentWorkspaceIndex(int index);

    QString currentWorkspaceName() const;
    QString currentWorkspaceToken() const;

    Q_INVOKABLE void addWorkspace(const QString &name,
                                  const QString &token,
                                  const QString &teamId,
                                  const QString &userId,
                                  const QString &domain);

    Q_INVOKABLE void removeWorkspace(int index);
    Q_INVOKABLE void switchWorkspace(int index);
    Q_INVOKABLE int workspaceCount() const { return m_workspaces.count(); }
    Q_INVOKABLE void removeDuplicates();

    void loadWorkspaces();
    void saveWorkspaces();

signals:
    void currentWorkspaceChanged();
    void workspaceAdded(const QString &name);
    void workspaceRemoved(int index);
    void workspaceSwitched(int index, const QString &token);
    void allWorkspacesRemoved();

private:
    QList<Workspace> m_workspaces;
    int m_currentWorkspaceIndex;

    void sortByLastUsed();
};

#endif // WORKSPACEMANAGER_H
