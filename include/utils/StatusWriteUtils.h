#ifndef STATUSWRITEUTILS_H
#define STATUSWRITEUTILS_H

#include <QVariantMap>
#include <QString>

namespace StatusWriteUtils {

inline void setStatus(QVariantMap& data, const QString& status)
{
    data["status"] = status;
}

inline void setUpdatedAt(QVariantMap& data, const QString& updatedAt)
{
    data["updated_at"] = updatedAt;
}

inline void setStatusAndUpdatedAt(QVariantMap& data, const QString& status, const QString& updatedAt)
{
    setStatus(data, status);
    setUpdatedAt(data, updatedAt);
}

inline void setErrorMessage(QVariantMap& data, const QString& errorMessage)
{
    if (!errorMessage.isEmpty()) {
        data["error_message"] = errorMessage;
    }
}

}

#endif // STATUSWRITEUTILS_H
