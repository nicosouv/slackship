#include "workspacemanager.h"
#include <QSettings>
#include <QJsonDocument>
#include <QDebug>

WorkspaceManager::WorkspaceManager(QObject *parent)
    : QAbstractListModel(parent)
    , m_currentWorkspaceIndex(-1)
{
    loadWorkspaces();
}

int WorkspaceManager::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_workspaces.count();
}

QVariant WorkspaceManager::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_workspaces.count())
        return QVariant();

    const Workspace &workspace = m_workspaces.at(index.row());

    switch (role) {
    case IdRole:
        return workspace.id;
    case NameRole:
        return workspace.name;
    case TokenRole:
        return workspace.token;
    case TeamIdRole:
        return workspace.teamId;
    case UserIdRole:
        return workspace.userId;
    case DomainRole:
        return workspace.domain;
    case IsActiveRole:
        return workspace.isActive;
    case LastUsedRole:
        return workspace.lastUsed;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> WorkspaceManager::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[NameRole] = "name";
    roles[TokenRole] = "token";
    roles[TeamIdRole] = "teamId";
    roles[UserIdRole] = "userId";
    roles[DomainRole] = "domain";
    roles[IsActiveRole] = "isActive";
    roles[LastUsedRole] = "lastUsed";
    return roles;
}

void WorkspaceManager::setCurrentWorkspaceIndex(int index)
{
    qDebug() << "[Workspace] setCurrentWorkspaceIndex called with index:" << index
             << "current:" << m_currentWorkspaceIndex
             << "count:" << m_workspaces.count();

    if (index < 0 || index >= m_workspaces.count()) {
        qWarning() << "[Workspace] Invalid index:" << index;
        return;
    }

    if (m_currentWorkspaceIndex != index) {
        QString targetTeamId = m_workspaces[index].teamId;
        QString targetName = m_workspaces[index].name;
        QString targetToken = m_workspaces[index].token;

        qDebug() << "[Workspace] Switching to:" << targetName << "teamId:" << targetTeamId;

        // Mark old workspace as inactive
        if (m_currentWorkspaceIndex >= 0 && m_currentWorkspaceIndex < m_workspaces.count()) {
            qDebug() << "[Workspace] Marking old workspace inactive:" << m_workspaces[m_currentWorkspaceIndex].name;
            m_workspaces[m_currentWorkspaceIndex].isActive = false;
        }

        // Mark new workspace as active
        m_currentWorkspaceIndex = index;
        m_workspaces[index].isActive = true;
        m_workspaces[index].lastUsed = QDateTime::currentDateTime();

        saveWorkspaces();
        sortByLastUsed();

        // After sorting, find the new index of the target workspace
        int newIndex = -1;
        for (int i = 0; i < m_workspaces.count(); ++i) {
            if (m_workspaces[i].teamId == targetTeamId) {
                newIndex = i;
                break;
            }
        }

        qDebug() << "[Workspace] After sort, new index:" << newIndex
                 << "currentWorkspaceIndex:" << m_currentWorkspaceIndex;

        emit currentWorkspaceChanged();
        emit workspaceSwitched(m_currentWorkspaceIndex, targetToken);

        qDebug() << "[Workspace] Switch complete, emitted workspaceSwitched with token length:" << targetToken.length();
    } else {
        qDebug() << "[Workspace] Already on this workspace, no switch needed";
    }
}

QString WorkspaceManager::currentWorkspaceName() const
{
    if (m_currentWorkspaceIndex >= 0 && m_currentWorkspaceIndex < m_workspaces.count()) {
        return m_workspaces.at(m_currentWorkspaceIndex).name;
    }
    return QString();
}

QString WorkspaceManager::currentWorkspaceToken() const
{
    if (m_currentWorkspaceIndex >= 0 && m_currentWorkspaceIndex < m_workspaces.count()) {
        return m_workspaces.at(m_currentWorkspaceIndex).token;
    }
    return QString();
}

void WorkspaceManager::addWorkspace(const QString &name,
                                   const QString &token,
                                   const QString &teamId,
                                   const QString &userId,
                                   const QString &domain)
{
    // Check if workspace already exists
    for (int i = 0; i < m_workspaces.count(); ++i) {
        if (m_workspaces[i].teamId == teamId && !teamId.isEmpty()) {
            qDebug() << "[Workspace] Updating:" << name;
            m_workspaces[i].token = token;
            m_workspaces[i].name = name;
            m_workspaces[i].userId = userId;
            m_workspaces[i].domain = domain;
            m_workspaces[i].lastUsed = QDateTime::currentDateTime();

            QModelIndex idx = createIndex(i, 0);
            emit dataChanged(idx, idx);
            saveWorkspaces();
            return;
        }
    }

    // Add new workspace
    qDebug() << "[Workspace] Adding:" << name;
    Workspace workspace;
    workspace.id = QUuid::createUuid().toString();
    workspace.name = name;
    workspace.token = token;
    workspace.teamId = teamId;
    workspace.userId = userId;
    workspace.domain = domain;
    workspace.isActive = false;
    workspace.lastUsed = QDateTime::currentDateTime();

    beginInsertRows(QModelIndex(), m_workspaces.count(), m_workspaces.count());
    m_workspaces.append(workspace);
    endInsertRows();

    saveWorkspaces();
    emit workspaceAdded(name);

    if (m_workspaces.count() == 1) {
        setCurrentWorkspaceIndex(0);
    }
}

void WorkspaceManager::removeWorkspace(int index)
{
    if (index < 0 || index >= m_workspaces.count())
        return;

    beginRemoveRows(QModelIndex(), index, index);
    m_workspaces.removeAt(index);
    endRemoveRows();

    // Update current workspace index if needed
    if (m_currentWorkspaceIndex == index) {
        m_currentWorkspaceIndex = -1;
        if (m_workspaces.count() > 0) {
            setCurrentWorkspaceIndex(0);
        } else {
            emit currentWorkspaceChanged();
            emit allWorkspacesRemoved();
        }
    } else if (m_currentWorkspaceIndex > index) {
        m_currentWorkspaceIndex--;
    }

    saveWorkspaces();
    emit workspaceRemoved(index);

    // Check if all workspaces have been removed
    if (m_workspaces.count() == 0) {
        emit allWorkspacesRemoved();
    }
}

void WorkspaceManager::switchWorkspace(int index)
{
    qDebug() << "[Workspace] switchWorkspace called with index:" << index;
    setCurrentWorkspaceIndex(index);
}

void WorkspaceManager::removeDuplicates()
{
    QHash<QString, int> teamIdToKeepIndex;
    QSet<int> indicesToRemove;

    for (int i = 0; i < m_workspaces.count(); ++i) {
        const QString &teamId = m_workspaces[i].teamId;

        if (teamId.isEmpty()) {
            indicesToRemove.insert(i);
            continue;
        }

        if (!teamIdToKeepIndex.contains(teamId)) {
            teamIdToKeepIndex[teamId] = i;
        } else {
            int existingIndex = teamIdToKeepIndex[teamId];
            if (m_workspaces[i].lastUsed > m_workspaces[existingIndex].lastUsed) {
                indicesToRemove.insert(existingIndex);
                teamIdToKeepIndex[teamId] = i;
            } else {
                indicesToRemove.insert(i);
            }
        }
    }

    if (indicesToRemove.isEmpty()) {
        return;
    }

    qDebug() << "[Workspace] Removing" << indicesToRemove.size() << "duplicates";

    QList<int> sortedIndices = indicesToRemove.toList();
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

    QString currentTeamId;
    if (m_currentWorkspaceIndex >= 0 && m_currentWorkspaceIndex < m_workspaces.count()) {
        currentTeamId = m_workspaces[m_currentWorkspaceIndex].teamId;
    }

    for (int index : sortedIndices) {
        beginRemoveRows(QModelIndex(), index, index);
        m_workspaces.removeAt(index);
        endRemoveRows();
    }

    if (!currentTeamId.isEmpty()) {
        m_currentWorkspaceIndex = -1;
        for (int i = 0; i < m_workspaces.count(); ++i) {
            if (m_workspaces[i].teamId == currentTeamId) {
                m_currentWorkspaceIndex = i;
                m_workspaces[i].isActive = true;
                break;
            }
        }
    } else {
        m_currentWorkspaceIndex = -1;
    }

    if (m_currentWorkspaceIndex == -1 && m_workspaces.count() > 0) {
        setCurrentWorkspaceIndex(0);
    }

    saveWorkspaces();
    beginResetModel();
    endResetModel();
}

void WorkspaceManager::loadWorkspaces()
{
    QSettings settings("harbour-lagoon", "workspaces");
    int count = settings.beginReadArray("workspaces");

    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);

        Workspace workspace;
        workspace.id = settings.value("id").toString();
        workspace.name = settings.value("name").toString();
        workspace.token = settings.value("token").toString();
        workspace.teamId = settings.value("teamId").toString();
        workspace.userId = settings.value("userId").toString();
        workspace.domain = settings.value("domain").toString();
        workspace.isActive = settings.value("isActive").toBool();
        workspace.lastUsed = settings.value("lastUsed").toDateTime();

        m_workspaces.append(workspace);

        if (workspace.isActive) {
            m_currentWorkspaceIndex = i;
        }
    }

    settings.endArray();

    sortByLastUsed();

    qDebug() << "[Workspace] Loaded" << count;
    removeDuplicates();
}

void WorkspaceManager::saveWorkspaces()
{
    QSettings settings("harbour-lagoon", "workspaces");
    settings.beginWriteArray("workspaces");

    for (int i = 0; i < m_workspaces.count(); ++i) {
        settings.setArrayIndex(i);
        const Workspace &workspace = m_workspaces.at(i);

        settings.setValue("id", workspace.id);
        settings.setValue("name", workspace.name);
        settings.setValue("token", workspace.token);
        settings.setValue("teamId", workspace.teamId);
        settings.setValue("userId", workspace.userId);
        settings.setValue("domain", workspace.domain);
        settings.setValue("isActive", workspace.isActive);
        settings.setValue("lastUsed", workspace.lastUsed);
    }

    settings.endArray();
    settings.sync();
}

void WorkspaceManager::sortByLastUsed()
{
    beginResetModel();

    std::sort(m_workspaces.begin(), m_workspaces.end(),
              [](const Workspace &a, const Workspace &b) -> bool {
        return a.lastUsed > b.lastUsed;
    });

    // Update current workspace index after sorting
    for (int i = 0; i < m_workspaces.count(); ++i) {
        if (m_workspaces[i].isActive) {
            m_currentWorkspaceIndex = i;
            break;
        }
    }

    endResetModel();
}

void WorkspaceManager::clearAllWorkspaces()
{
    if (m_workspaces.isEmpty()) {
        return;
    }

    qDebug() << "[Workspace] Clearing all";
    beginResetModel();
    m_workspaces.clear();
    m_currentWorkspaceIndex = -1;
    endResetModel();

    saveWorkspaces();
    emit currentWorkspaceChanged();
    emit allWorkspacesRemoved();
}
