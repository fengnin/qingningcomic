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
    
    inline QVector<EnumDefinition> getShotTypeDefinitions()
    {
        QVector<EnumDefinition> defs;
        defs.append(EnumDefinition(QString::fromUtf8("远景"), "wide", QStringList() << QString::fromUtf8("远") << "wide" << "long"));
        defs.append(EnumDefinition(QString::fromUtf8("中景"), "medium", QStringList() << QString::fromUtf8("中") << "medium"));
        defs.append(EnumDefinition(QString::fromUtf8("近景"), "close-up", QStringList() << QString::fromUtf8("近") << "close"));
        defs.append(EnumDefinition(QString::fromUtf8("特写"), "closeup", QStringList() << QString::fromUtf8("特") << "closeup" << "extreme"));
        defs.append(EnumDefinition(QString::fromUtf8("全景"), "longshot", QStringList() << QString::fromUtf8("全") << "full" << "longshot"));
        defs.append(EnumDefinition(QString::fromUtf8("远全景"), "panoramic", QStringList() << "wide-shot" << "panoramic"));
        return defs;
    }
    
    inline QVector<EnumDefinition> getCameraAngleDefinitions()
    {
        QVector<EnumDefinition> defs;
        defs.append(EnumDefinition(QString::fromUtf8("俯视"), "high-angle", QStringList() << QString::fromUtf8("俯") << "high" << "top"));
        defs.append(EnumDefinition(QString::fromUtf8("仰视"), "low-angle", QStringList() << QString::fromUtf8("仰") << "low" << "bottom"));
        defs.append(EnumDefinition(QString::fromUtf8("平视"), "eye-level", QStringList() << QString::fromUtf8("平") << "eye" << "level" << QString::fromUtf8("侧") << QString::fromUtf8("正") << QString::fromUtf8("跟")));
        defs.append(EnumDefinition(QString::fromUtf8("倾斜"), "dutch", QStringList() << QString::fromUtf8("倾") << "dutch" << "tilt"));
        defs.append(EnumDefinition(QString::fromUtf8("鸟瞰"), "bird-eye", QStringList() << QString::fromUtf8("鸟") << "bird" << "birdview"));
        defs.append(EnumDefinition(QString::fromUtf8("虫视"), "worm-eye", QStringList() << QString::fromUtf8("虫") << "worm"));
        return defs;
    }
    
    inline QString normalizeByDefinitions(const QString& input, const QVector<EnumDefinition>& defs, const QString& defaultVal)
    {
        if (input.isEmpty()) return input;
        
        QString normalized = input.trimmed().toLower();
        
        int bestMatchIndex = -1;
        int bestMatchLength = 0;
        
        for (int i = 0; i < defs.size(); ++i) {
            const EnumDefinition& def = defs.at(i);
            if (normalized.contains(def.chinese)) {
                int len = def.chinese.length();
                if (len > bestMatchLength) {
                    bestMatchLength = len;
                    bestMatchIndex = i;
                }
            }
        }
        
        if (bestMatchIndex >= 0) {
            return defs.at(bestMatchIndex).chinese;
        }
        
        for (int i = 0; i < defs.size(); ++i) {
            const EnumDefinition& def = defs.at(i);
            if (normalized.contains(def.english.toLower())) {
                return def.chinese;
            }
        }
        
        for (int i = 0; i < defs.size(); ++i) {
            const EnumDefinition& def = defs.at(i);
            for (int j = 0; j < def.keywords.size(); ++j) {
                if (normalized.contains(def.keywords.at(j).toLower())) {
                    return def.chinese;
                }
            }
        }
        
        return defaultVal;
    }
}

inline QStringList shotTypeStandardList()
{
    static QStringList list = QStringList()
        << QString::fromUtf8("远景")
        << QString::fromUtf8("中景")
        << QString::fromUtf8("近景")
        << QString::fromUtf8("特写")
        << QString::fromUtf8("全景")
        << QString::fromUtf8("远全景");
    return list;
}

inline QStringList cameraAngleStandardList()
{
    static QStringList list = QStringList()
        << QString::fromUtf8("俯视")
        << QString::fromUtf8("仰视")
        << QString::fromUtf8("平视")
        << QString::fromUtf8("倾斜")
        << QString::fromUtf8("鸟瞰")
        << QString::fromUtf8("虫视");
    return list;
}

namespace ShotType {
    
    inline QStringList standardList() { return shotTypeStandardList(); }
    
    inline QString toChinese(const QString& english)
    {
        QString normalized = english.trimmed().toLower();
        QVector<EnumDefinition> defs = Detail::getShotTypeDefinitions();
        
        for (int i = 0; i < defs.size(); ++i) {
            if (defs.at(i).english.toLower() == normalized || normalized.contains(defs.at(i).english.toLower())) {
                return defs.at(i).chinese;
            }
        }
        for (int i = 0; i < defs.size(); ++i) {
            for (int j = 0; j < defs.at(i).keywords.size(); ++j) {
                if (defs.at(i).keywords.at(j).toLower() == normalized || normalized.contains(defs.at(i).keywords.at(j).toLower())) {
                    return defs.at(i).chinese;
                }
            }
        }
        return english;
    }
    
    inline QString toEnglish(const QString& chinese)
    {
        QString normalized = chinese.trimmed();
        QVector<EnumDefinition> defs = Detail::getShotTypeDefinitions();
        
        for (int i = 0; i < defs.size(); ++i) {
            if (defs.at(i).chinese == normalized) {
                return defs.at(i).english;
            }
        }
        return chinese.toLower();
    }
    
    inline QString normalize(const QString& input)
    {
        return Detail::normalizeByDefinitions(input, Detail::getShotTypeDefinitions(), QString::fromUtf8("中景"));
    }
}

namespace CameraAngle {
    
    inline QStringList standardList() { return cameraAngleStandardList(); }
    
    inline QString toChinese(const QString& english)
    {
        QString normalized = english.trimmed().toLower();
        QVector<EnumDefinition> defs = Detail::getCameraAngleDefinitions();
        
        for (int i = 0; i < defs.size(); ++i) {
            if (defs.at(i).english.toLower() == normalized || normalized.contains(defs.at(i).english.toLower())) {
                return defs.at(i).chinese;
            }
        }
        for (int i = 0; i < defs.size(); ++i) {
            for (int j = 0; j < defs.at(i).keywords.size(); ++j) {
                if (defs.at(i).keywords.at(j).toLower() == normalized || normalized.contains(defs.at(i).keywords.at(j).toLower())) {
                    return defs.at(i).chinese;
                }
            }
        }
        return english;
    }
    
    inline QString toEnglish(const QString& chinese)
    {
        QString normalized = chinese.trimmed();
        QVector<EnumDefinition> defs = Detail::getCameraAngleDefinitions();
        
        for (int i = 0; i < defs.size(); ++i) {
            if (defs.at(i).chinese == normalized) {
                return defs.at(i).english;
            }
        }
        return chinese.toLower();
    }
    
    inline QString normalize(const QString& input)
    {
        return Detail::normalizeByDefinitions(input, Detail::getCameraAngleDefinitions(), QString::fromUtf8("平视"));
    }
}

}

#endif // SHOTTYPEHELPER_H
