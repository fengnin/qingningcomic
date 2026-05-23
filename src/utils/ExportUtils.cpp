#include "utils/ExportUtils.h"
#include "models/Panel.h"

#include <algorithm>
#include <QRegularExpression>

namespace ExportUtils {

namespace {
QString replaceKnownOcrArtifacts(QString text)
{
    static const QList<QPair<QRegularExpression, QString>> replacements = {
        {QRegularExpression(QStringLiteral(R"(page\(i=Pang1)"), QRegularExpression::CaseInsensitiveOption), QString()},
        {QRegularExpression(QStringLiteral(R"(Page\s*1\s*\.?\s*Panel\s*\d+)"), QRegularExpression::CaseInsensitiveOption), QString()},
        {QRegularExpression(QStringLiteral(R"(高清成品)")), QString()},
        {QRegularExpression(QStringLiteral(R"(第\s*1\s*章\s*[·\.]\s*面板\s*\d+\s*[·\.]\s*第\s*1\s*页)")), QString()},
        {QRegularExpression(QStringLiteral(R"(骑使)")), QString::fromUtf8("骑楼")},
        {QRegularExpression(QStringLiteral(R"(光瓶)")), QString::fromUtf8("光斑")},
        {QRegularExpression(QStringLiteral(R"(素描1)")), QString::fromUtf8("素描本")},
        {QRegularExpression(QStringLiteral(R"(薄荷槽)")), QString::fromUtf8("薄荷糖")},
        {QRegularExpression(QStringLiteral(R"(有限公司内容再响)")), QString::fromUtf8("店内安静下来")},
        {QRegularExpression(QStringLiteral(R"(请拧含着)")), QString::fromUtf8("她嘴里含着")},
        {QRegularExpression(QStringLiteral(R"(装在她睡至上)")), QString::fromUtf8("挂在她睡衣上")},
        {QRegularExpression(QStringLiteral(R"(像偏胖了一小长六日的风)")), QString::fromUtf8("像被风轻轻吹乱的发梢")}
    };

    for (const auto& pair : replacements) {
        text.replace(pair.first, pair.second);
    }
    return text;
}
}

QString exportFormatToString(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:
        return QStringLiteral("pdf");
    case ExportFormat::Webtoon:
        return QStringLiteral("webtoon");
    case ExportFormat::Resources:
        return QStringLiteral("resources");
    }
    return QStringLiteral("pdf");
}

QString exportFormatLabel(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:
        return QStringLiteral("PDF");
    case ExportFormat::Webtoon:
        return QStringLiteral("Webtoon 长图");
    case ExportFormat::Resources:
        return QStringLiteral("资源包 (ZIP)");
    }
    return QStringLiteral("PDF");
}

QString exportFormatLabel(const QString& formatStr)
{
    if (formatStr == QLatin1String("pdf"))       return QStringLiteral("PDF");
    if (formatStr == QLatin1String("webtoon"))   return QStringLiteral("Webtoon 长图");
    if (formatStr == QLatin1String("resources")) return QStringLiteral("资源包 (ZIP)");
    return formatStr;
}

QString exportFileExtension(ExportFormat format)
{
    switch (format) {
    case ExportFormat::Pdf:
        return QStringLiteral("pdf");
    case ExportFormat::Webtoon:
        return QStringLiteral("png");
    case ExportFormat::Resources:
        return QStringLiteral("zip");
    }
    return QStringLiteral("pdf");
}

QList<Panel> sortPanelsForExport(const QList<Panel>& panels)
{
    QList<Panel> sorted = panels;
    std::sort(sorted.begin(), sorted.end(), [](const Panel& a, const Panel& b) {
        if (a.page() != b.page()) {
            return a.page() < b.page();
        }
        return a.index() < b.index();
    });
    return sorted;
}

QString panelBaseName(const Panel& panel, int index)
{
    const int seq = index + 1;
    return QStringLiteral("%1-p%2-i%3-%4")
        .arg(seq, 3, 10, QLatin1Char('0'))
        .arg(panel.page(), 2, 10, QLatin1Char('0'))
        .arg(panel.index(), 2, 10, QLatin1Char('0'))
        .arg(panel.id().toLower().simplified().replace(QLatin1Char(' '), QLatin1Char('-')).left(60));
}

QString panelDisplayIndex(const Panel& panel)
{
    const int index = panel.index() > 0 ? panel.index() : 0;
    return QString::number(index + 1);
}

QString exportPanelHeaderText(const QString& novelTitle, int chapterNumber, int panelNumber)
{
    return QStringLiteral("%1 · 第 %2 章 · 面板 %3")
        .arg(novelTitle.isEmpty() ? QStringLiteral("未命名作品") : novelTitle)
        .arg(chapterNumber)
        .arg(panelNumber);
}

QString sanitizeExportText(const QString& text)
{
    if (text.isEmpty()) {
        return text;
    }

    QString result = text;
    result = replaceKnownOcrArtifacts(result);
    result.replace(QRegularExpression(QStringLiteral(R"(\s{2,})")), QStringLiteral(" "));
    result.replace(QRegularExpression(QStringLiteral(R"([·\.]{2,})")), QStringLiteral("·"));
    result.replace(QRegularExpression(QStringLiteral(R"(^[\s·\.]+)")), QString());
    result.replace(QRegularExpression(QStringLiteral(R"([\s·\.]+$)")), QString());
    return result.trimmed();
}

}
