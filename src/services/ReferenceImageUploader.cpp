#include "services/ReferenceImageUploader.h"
#include "utils/AppConfig.h"
#include "utils/Logger.h"

#include <QEventLoop>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUuid>

namespace {
constexpr int kUploadTimeoutMs = 120000;

QString firstUrlFromObject(const QJsonObject& object)
{
    const QStringList keys = {
        QStringLiteral("url"),
        QStringLiteral("publicUrl"),
        QStringLiteral("imageUrl"),
        QStringLiteral("downloadUrl")
    };

    for (const QString& key : keys) {
        const QString value = object.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }

    const QJsonObject data = object.value(QStringLiteral("data")).toObject();
    for (const QString& key : keys) {
        const QString value = data.value(key).toString().trimmed();
        if (!value.isEmpty()) {
            return value;
        }
    }

    return QString();
}

QString buildUploadFileName(const QString& requestedName)
{
    const QString suffix = QFileInfo(requestedName).suffix().isEmpty()
        ? QStringLiteral("png")
        : QFileInfo(requestedName).suffix();
    const QString baseName = QFileInfo(requestedName).completeBaseName().isEmpty()
        ? QUuid::createUuid().toString(QUuid::WithoutBraces)
        : QFileInfo(requestedName).completeBaseName();
    return QStringLiteral("%1.%2").arg(baseName, suffix);
}
}

ReferenceImageUploader::UploadResult ReferenceImageUploader::uploadImage(const QByteArray& imageData,
                                                                         const QString& fileName,
                                                                         const QString& contentType)
{
    UploadResult result;
    if (imageData.isEmpty()) {
        result.errorMessage = QStringLiteral("Reference image data is empty");
        return result;
    }

    const AppConfig::StorageConfig storage = AppConfig::instance()->storage();
    const QString endpoint = storage.presignApiEndpoint.trimmed();
    if (endpoint.isEmpty()) {
        result.errorMessage = QStringLiteral("storage.presignApiEndpoint is not configured");
        return result;
    }

    QNetworkAccessManager manager;
    QUrl requestUrl(endpoint);
    QNetworkRequest request(requestUrl);
    if (!storage.authToken.trimmed().isEmpty()) {
        request.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(storage.authToken.trimmed()).toUtf8());
    }

    QHttpMultiPart* multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    const QString uploadFileName = buildUploadFileName(fileName);
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"file\"; filename=\"%1\"").arg(uploadFileName));
    filePart.setBody(imageData);
    multiPart->append(filePart);

    QHttpPart namePart;
    namePart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"filename\""));
    namePart.setBody(uploadFileName.toUtf8());
    multiPart->append(namePart);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    QNetworkReply* reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });

    timer.start(kUploadTimeoutMs);
    loop.exec();

    // 统一错误返回：记录日志、释放reply、返回失败结果
    auto fail = [&](const QString& msg, const QByteArray& body = {}) -> UploadResult {
        result.errorMessage = msg;
        LOG_ERROR("ReferenceImageUploader", body.isEmpty()
            ? msg
            : QString("%1: %2").arg(msg, QString::fromUtf8(body.left(300))));
        reply->deleteLater();
        return result;
    };

    const QByteArray responseBody = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        return fail(QStringLiteral("Reference image upload failed: %1").arg(reply->errorString()), responseBody);
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(responseBody, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return fail(QStringLiteral("Reference image upload response is not valid JSON"), responseBody);
    }

    result.url = firstUrlFromObject(document.object());
    if (result.url.isEmpty()) {
        return fail(QStringLiteral("Reference image upload response does not contain a public URL"), responseBody);
    }

    result.success = true;
    LOG_INFO("ReferenceImageUploader", QString("Reference image uploaded: %1").arg(result.url.left(120)));
    reply->deleteLater();
    return result;
}
