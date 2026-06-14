#ifndef REFERENCEIMAGEUPLOADER_H
#define REFERENCEIMAGEUPLOADER_H

#include <QByteArray>
#include <QString>

class ReferenceImageUploader
{
public:
    struct UploadResult {
        bool success = false;
        QString url;
        QString errorMessage;
    };

    static UploadResult uploadImage(const QByteArray& imageData,
                                    const QString& fileName,
                                    const QString& contentType = QStringLiteral("image/png"));
};

#endif // REFERENCEIMAGEUPLOADER_H
