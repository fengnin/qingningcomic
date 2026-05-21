#ifndef SHOTTYPEHELPER_H
#define SHOTTYPEHELPER_H

#include <QString>
#include <QMap>
#include <QStringList>
#include <QVector>

namespace ShotTypeHelper {

struct EnumDefinition {
    QString chinese;
    QString english;
    QStringList keywords;
    
    EnumDefinition() {}
    EnumDefinition(const QString& c, const QString& e, const QStringList& k)
        : chinese(c), english(e), keywords(k) {}
};

struct ShotTypeTag {};
struct CameraAngleTag {};

namespace Detail {
    
    const QVector<EnumDefinition>& getShotTypeDefinitions();
    
    const QVector<EnumDefinition>& getCameraAngleDefinitions();
    
    QString normalizeByDefinitions(const QString& input, const QVector<EnumDefinition>& defs, const QString& defaultVal);
}

QStringList shotTypeStandardList();

QStringList cameraAngleStandardList();

namespace ShotType {
    
    inline QStringList standardList() { return shotTypeStandardList(); }
    
    QString toChinese(const QString& english);
    
    QString toEnglish(const QString& chinese);
    
    QString normalize(const QString& input);
}

namespace CameraAngle {
    
    inline QStringList standardList() { return cameraAngleStandardList(); }
    
    QString toChinese(const QString& english);
    
    QString toEnglish(const QString& chinese);
    
    QString normalize(const QString& input);
}

}

#endif // SHOTTYPEHELPER_H
