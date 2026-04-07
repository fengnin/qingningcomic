#ifndef SHOTTYPEHELPER_H
#define SHOTTYPEHELPER_H

#include <QString>
#include <QMap>

namespace ShotTypeHelper {

namespace ShotType {
    inline QString toChinese(const QString& english) {
        static const QMap<QString, QString> mapping = {
            {"wide", QString::fromUtf8("广角")},
            {"medium", QString::fromUtf8("中景")},
            {"close-up", QString::fromUtf8("近景")},
            {"closeup", QString::fromUtf8("特写")},
            {"long", QString::fromUtf8("远景")},
            {"panoramic", QString::fromUtf8("全景")},
            {"extreme close-up", QString::fromUtf8("特写")}
        };
        return mapping.value(english.toLower(), english);
    }
    
    inline QString toEnglish(const QString& chinese) {
        static const QMap<QString, QString> mapping = {
            {QString::fromUtf8("广角"), "wide"},
            {QString::fromUtf8("中景"), "medium"},
            {QString::fromUtf8("近景"), "close-up"},
            {QString::fromUtf8("特写"), "closeup"},
            {QString::fromUtf8("远景"), "long"},
            {QString::fromUtf8("全景"), "panoramic"}
        };
        return mapping.value(chinese, chinese.toLower());
    }
    
    inline QStringList chineseList() {
        return {
            QString::fromUtf8("广角"),
            QString::fromUtf8("中景"),
            QString::fromUtf8("近景"),
            QString::fromUtf8("特写"),
            QString::fromUtf8("远景"),
            QString::fromUtf8("全景")
        };
    }
}

namespace CameraAngle {
    inline QString toChinese(const QString& english) {
        static const QMap<QString, QString> mapping = {
            {"high-angle", QString::fromUtf8("俯视")},
            {"high angle", QString::fromUtf8("俯视")},
            {"low-angle", QString::fromUtf8("仰视")},
            {"low angle", QString::fromUtf8("仰视")},
            {"eye-level", QString::fromUtf8("平视")},
            {"eye level", QString::fromUtf8("平视")},
            {"dutch", QString::fromUtf8("荷兰角")},
            {"bird-eye", QString::fromUtf8("鸟眼")},
            {"bird eye", QString::fromUtf8("鸟眼")},
            {"worm-eye", QString::fromUtf8("虫视")},
            {"worm eye", QString::fromUtf8("虫视")}
        };
        return mapping.value(english.toLower(), english);
    }
    
    inline QString toEnglish(const QString& chinese) {
        static const QMap<QString, QString> mapping = {
            {QString::fromUtf8("俯视"), "high-angle"},
            {QString::fromUtf8("仰视"), "low-angle"},
            {QString::fromUtf8("平视"), "eye-level"},
            {QString::fromUtf8("荷兰角"), "dutch"},
            {QString::fromUtf8("鸟眼"), "bird-eye"},
            {QString::fromUtf8("虫视"), "worm-eye"}
        };
        return mapping.value(chinese, chinese.toLower());
    }
    
    inline QStringList chineseList() {
        return {
            QString::fromUtf8("俯视"),
            QString::fromUtf8("仰视"),
            QString::fromUtf8("平视"),
            QString::fromUtf8("荷兰角"),
            QString::fromUtf8("鸟眼"),
            QString::fromUtf8("虫视")
        };
    }
}

}

#endif // SHOTTYPEHELPER_H
