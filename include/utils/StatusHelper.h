#ifndef STATUSHELPER_H
#define STATUSHELPER_H

#include <QString>

namespace StatusHelper {

struct StatusStyle {
    QString text;
    QString bgColor;
    QString textColor;
};

namespace Novel {
    QString label(const QString& status);
    StatusStyle style(const QString& status);
}

namespace Job {
    QString typeLabel(const QString& type);
    QString statusLabel(const QString& status);
}

namespace Chapter {
    QString label(const QString& status);
    StatusStyle style(const QString& status);
}

}

#endif // STATUSHELPER_H
