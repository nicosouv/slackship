#include "filemanager.h"
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QDebug>
#include <QMimeDatabase>
#include <QStandardPaths>

FileManager::FileManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentUpload(nullptr)
    , m_currentDownload(nullptr)
{
}

FileManager::~FileManager()
{
    if (m_currentUpload) {
        m_currentUpload->abort();
    }
    if (m_currentDownload) {
        m_currentDownload->abort();
    }
}

void FileManager::setToken(const QString &token)
{
    m_token = token;
}

void FileManager::uploadFile(const QString &channelId,
                            const QString &filePath,
                            const QString &comment)
{
    if (m_token.isEmpty()) {
        emit uploadError("Not authenticated");
        return;
    }

    QFile file(filePath);
    if (!file.exists()) {
        emit uploadError("File does not exist: " + filePath);
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadError("Cannot open file: " + filePath);
        return;
    }

    QFileInfo fileInfo(filePath);
    m_currentFileName = fileInfo.fileName();

    emit uploadStarted(m_currentFileName);

    // Create multipart form data
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Add file part
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
                      QMimeDatabase().mimeTypeForFile(filePath).name());
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                      QVariant(QString("form-data; name=\"file\"; filename=\"%1\"")
                              .arg(m_currentFileName)));
    filePart.setBodyDevice(&file);
    file.setParent(multiPart);
    multiPart->append(filePart);

    // Add channels part
    QHttpPart channelsPart;
    channelsPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant("form-data; name=\"channels\""));
    channelsPart.setBody(channelId.toUtf8());
    multiPart->append(channelsPart);

    // Add initial comment if provided
    if (!comment.isEmpty()) {
        QHttpPart commentPart;
        commentPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                             QVariant("form-data; name=\"initial_comment\""));
        commentPart.setBody(comment.toUtf8());
        multiPart->append(commentPart);
    }

    // Create request
    QUrl url("https://slack.com/api/files.upload");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());

    // Send request
    m_currentUpload = m_networkManager->post(request, multiPart);
    multiPart->setParent(m_currentUpload);

    connect(m_currentUpload, &QNetworkReply::finished,
            this, &FileManager::handleUploadFinished);
    connect(m_currentUpload, &QNetworkReply::uploadProgress,
            this, &FileManager::handleUploadProgress);
    connect(m_currentUpload, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &FileManager::handleUploadError);
}

void FileManager::uploadImage(const QString &channelId,
                             const QString &imagePath,
                             const QString &comment)
{
    uploadFile(channelId, imagePath, comment);
}

void FileManager::downloadFile(const QString &fileId,
                              const QString &privateUrl,
                              const QString &savePath)
{
    if (m_token.isEmpty()) {
        emit downloadError("Not authenticated");
        return;
    }

    QString finalPath = savePath;
    if (finalPath.isEmpty()) {
        QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        finalPath = downloadsPath + "/" + fileId;
    }

    QNetworkRequest request(privateUrl);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_token).toUtf8());

    m_currentDownload = m_networkManager->get(request);
    m_currentFileName = QFileInfo(finalPath).fileName();

    emit downloadStarted(m_currentFileName);

    connect(m_currentDownload, &QNetworkReply::finished,
            this, &FileManager::handleDownloadFinished);
    connect(m_currentDownload, &QNetworkReply::downloadProgress,
            this, &FileManager::handleDownloadProgress);
    connect(m_currentDownload, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &FileManager::handleDownloadError);
}

void FileManager::downloadImage(const QString &imageUrl, const QString &savePath)
{
    downloadFile(QString(), imageUrl, savePath);
}

void FileManager::cancelUpload()
{
    if (m_currentUpload) {
        m_currentUpload->abort();
        m_currentUpload->deleteLater();
        m_currentUpload = nullptr;
    }
}

void FileManager::cancelDownload(const QString &fileId)
{
    Q_UNUSED(fileId);
    if (m_currentDownload) {
        m_currentDownload->abort();
        m_currentDownload->deleteLater();
        m_currentDownload = nullptr;
    }
}

void FileManager::handleUploadFinished()
{
    QNetworkReply *reply = m_currentUpload;
    reply->deleteLater();
    m_currentUpload = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        return; // Error already handled by handleUploadError
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (!doc.isObject()) {
        emit uploadError("Invalid response from server");
        return;
    }

    QJsonObject response = doc.object();

    if (!response["ok"].toBool()) {
        QString error = response["error"].toString();
        emit uploadError(error);
        return;
    }

    QJsonObject fileInfo = response["file"].toObject();
    QString fileId = fileInfo["id"].toString();

    emit uploadFinished(fileId, fileInfo);
    qDebug() << "File uploaded successfully:" << fileId;
}

void FileManager::handleUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    emit uploadProgress(m_currentFileName, bytesSent, bytesTotal);
}

void FileManager::handleUploadError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        emit uploadError(reply->errorString());
    }
}

void FileManager::handleDownloadFinished()
{
    QNetworkReply *reply = m_currentDownload;
    reply->deleteLater();
    m_currentDownload = nullptr;

    if (reply->error() != QNetworkReply::NoError) {
        return; // Error already handled by handleDownloadError
    }

    // Save file to disk
    QString downloadsPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QString filePath = downloadsPath + "/" + m_currentFileName;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit downloadError("Cannot save file: " + filePath);
        return;
    }

    file.write(reply->readAll());
    file.close();

    emit downloadFinished(filePath);
    qDebug() << "File downloaded successfully to:" << filePath;
}

void FileManager::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    emit downloadProgress(m_currentFileName, bytesReceived, bytesTotal);
}

void FileManager::handleDownloadError(QNetworkReply::NetworkError error)
{
    Q_UNUSED(error);
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        emit downloadError(reply->errorString());
    }
}

QByteArray FileManager::createMultipartData(const QString &filePath,
                                           const QString &channelId,
                                           const QString &comment,
                                           QByteArray &boundary)
{
    boundary = "----LagoonBoundary" + QByteArray::number(qrand());

    QByteArray data;
    data.append("--" + boundary + "\r\n");
    data.append("Content-Disposition: form-data; name=\"channels\"\r\n\r\n");
    data.append(channelId.toUtf8() + "\r\n");

    if (!comment.isEmpty()) {
        data.append("--" + boundary + "\r\n");
        data.append("Content-Disposition: form-data; name=\"initial_comment\"\r\n\r\n");
        data.append(comment.toUtf8() + "\r\n");
    }

    // File part
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly)) {
        QFileInfo fileInfo(filePath);
        data.append("--" + boundary + "\r\n");
        data.append(QString("Content-Disposition: form-data; name=\"file\"; filename=\"%1\"\r\n")
                   .arg(fileInfo.fileName()).toUtf8());
        data.append("Content-Type: application/octet-stream\r\n\r\n");
        data.append(file.readAll());
        data.append("\r\n");
        file.close();
    }

    data.append("--" + boundary + "--\r\n");

    return data;
}
