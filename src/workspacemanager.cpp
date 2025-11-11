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
    if (index < 0 || index >= m_workspaces.count())
        return;

    if (m_currentWorkspaceIndex != index) {
        // Mark old workspace as inactive
        if (m_currentWorkspaceIndex >= 0 && m_currentWorkspaceIndex < m_workspaces.count()) {
            m_workspaces[m_currentWorkspaceIndex].isActive = false;
        }

        // Mark new workspace as active
        m_currentWorkspaceIndex = index;
        m_workspaces[index].isActive = true;
        m_workspaces[index].lastUsed = QDateTime::currentDateTime();

        saveWorkspaces();
        sortByLastUsed();

        emit currentWorkspaceChanged();
        emit workspaceSwitched(index, m_workspaces[index].token);
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
    qDebug() << "=== ADD WORKSPACE CALLED ===";
    qDebug() << "Name:" << name;
    qDebug() << "TeamId:" << teamId;
    qDebug() << "TeamId isEmpty:" << teamId.isEmpty();
    qDebug() << "Current workspace count:" << m_workspaces.count();

    // Check if workspace already exists
    for (int i = 0; i < m_workspaces.count(); ++i) {
        qDebug() << "Checking workspace" << i << "- stored teamId:" << m_workspaces[i].teamId;
        if (m_workspaces[i].teamId == teamId && !teamId.isEmpty()) {
            qDebug() << "Found existing workspace at index" << i << "- UPDATING";
            // Update existing workspace
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
    qDebug() << "No existing workspace found - ADDING NEW";
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
    qDebug() << "Workspace added - new count:" << m_workspaces.count();

    // If this is the first workspace, make it active
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
    setCurrentWorkspaceIndex(index);
}

void WorkspaceManager::removeDuplicates()
{
    qDebug() << "=== REMOVING DUPLICATE WORKSPACES ===";
    qDebug() << "Starting with" << m_workspaces.count() << "workspaces";

    // Log all workspaces before cleanup
    for (int i = 0; i < m_workspaces.count(); ++i) {
        qDebug() << "  [" << i << "]"
                 << "name:" << m_workspaces[i].name
                 << "teamId:" << m_workspaces[i].teamId
                 << "lastUsed:" << m_workspaces[i].lastUsed.toString();
    }

    // Build list of indices to keep (one per unique teamId)
    QHash<QString, int> teamIdToKeepIndex;  // teamId -> index to keep
    QSet<int> indicesToRemove;

    for (int i = 0; i < m_workspaces.count(); ++i) {
        const QString &teamId = m_workspaces[i].teamId;

        // Remove workspaces without teamId
        if (teamId.isEmpty()) {
            qDebug() << "Workspace at index" << i << "has no teamId, will remove";
            indicesToRemove.insert(i);
            continue;
        }

        if (!teamIdToKeepIndex.contains(teamId)) {
            // First occurrence of this teamId - keep it
            teamIdToKeepIndex[teamId] = i;
            qDebug() << "First occurrence of teamId" << teamId << "at index" << i << "- keeping";
        } else {
            // Duplicate found
            int existingIndex = teamIdToKeepIndex[teamId];

            // Compare lastUsed dates - keep the more recent one
            if (m_workspaces[i].lastUsed > m_workspaces[existingIndex].lastUsed) {
                qDebug() << "Duplicate teamId" << teamId
                         << "- new at index" << i << "is newer, removing old at" << existingIndex;
                indicesToRemove.insert(existingIndex);
                teamIdToKeepIndex[teamId] = i;
            } else {
                qDebug() << "Duplicate teamId" << teamId
                         << "- existing at index" << existingIndex << "is newer, removing new at" << i;
                indicesToRemove.insert(i);
            }
        }
    }

    if (indicesToRemove.isEmpty()) {
        qDebug() << "No duplicates found";
        return;
    }

    qDebug() << "Will remove" << indicesToRemove.size() << "duplicate/invalid workspaces";

    // Convert to sorted list (descending) to remove from end first
    QList<int> sortedIndices = indicesToRemove.toList();
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());

    // Remember current workspace teamId
    QString currentTeamId;
    if (m_currentWorkspaceIndex >= 0 && m_currentWorkspaceIndex < m_workspaces.count()) {
        currentTeamId = m_workspaces[m_currentWorkspaceIndex].teamId;
    }

    // Remove duplicates from end to beginning
    for (int index : sortedIndices) {
        qDebug() << "Removing workspace at index" << index
                 << "name:" << m_workspaces[index].name;

        beginRemoveRows(QModelIndex(), index, index);
        m_workspaces.removeAt(index);
        endRemoveRows();
    }

    // Find new index of current workspace
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

    // If no current workspace, select the first one
    if (m_currentWorkspaceIndex == -1 && m_workspaces.count() > 0) {
        setCurrentWorkspaceIndex(0);
    }

    saveWorkspaces();

    // Force complete model reset to ensure UI updates
    beginResetModel();
    endResetModel();

    qDebug() << "Cleanup complete - now have" << m_workspaces.count() << "workspaces";
    for (int i = 0; i < m_workspaces.count(); ++i) {
        qDebug() << "  [" << i << "]"
                 << "name:" << m_workspaces[i].name
                 << "teamId:" << m_workspaces[i].teamId;
    }
    qDebug() << "Model reset signal sent - UI should refresh";
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

    qDebug() << "Loaded" << count << "workspaces";

    // Auto-cleanup duplicates on load
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

    qDebug() << "Saved" << m_workspaces.count() << "workspaces";
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
