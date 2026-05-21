#ifndef CHANGEREQUESTTARGETUTILS_H
#define CHANGEREQUESTTARGETUTILS_H

#include <QList>
#include <QRegularExpression>
#include <QString>
#include <QVariantMap>

namespace ChangeRequestTargetUtils {

inline QString normalizePanelReferenceText(QString text)
{
    text.replace(QChar(0xFF10), QLatin1Char('0'));
    text.replace(QChar(0xFF11), QLatin1Char('1'));
    text.replace(QChar(0xFF12), QLatin1Char('2'));
    text.replace(QChar(0xFF13), QLatin1Char('3'));
    text.replace(QChar(0xFF14), QLatin1Char('4'));
    text.replace(QChar(0xFF15), QLatin1Char('5'));
    text.replace(QChar(0xFF16), QLatin1Char('6'));
    text.replace(QChar(0xFF17), QLatin1Char('7'));
    text.replace(QChar(0xFF18), QLatin1Char('8'));
    text.replace(QChar(0xFF19), QLatin1Char('9'));
    text.replace(QChar(0x500B), QStringLiteral("个"));
    text.remove(QRegularExpression(QStringLiteral(R"(\s+)")));
    return text;
}

int chineseNumeralToInt(const QString& text);

inline int panelNumberFromPageAndIndex(int page, int index)
{
    if (page <= 0 || index < 0) {
        return 0;
    }
    return (page - 1) * 6 + index + 1;
}

int extractRequestedPanelNumber(const QString& text);

inline QString resolvePanelIdByNumber(const QList<QVariantMap>& rows, int targetPanelNumber)
{
    if (targetPanelNumber <= 0) {
        return QString();
    }

    for (const QVariantMap& row : rows) {
        const int page = row.value("page").toInt();
        const int panelIndex = row.value("panel_index").toInt();
        const int panelNumber = panelNumberFromPageAndIndex(page, panelIndex);
        if (panelNumber == targetPanelNumber) {
            return row.value("id").toString();
        }
    }

    return QString();
}

}

#endif // CHANGEREQUESTTARGETUTILS_H
