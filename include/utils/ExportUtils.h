#ifndef EXPORTUTILS_H
#define EXPORTUTILS_H

#include <QString>
#include <QList>

class Panel;

namespace ExportUtils {

enum class ExportFormat {
    Pdf,
    Webtoon,
    Resources
};

QString exportFormatToString(ExportFormat format);
QString exportFormatLabel(ExportFormat format);
QString exportFormatLabel(const QString& formatStr);
QString exportFileExtension(ExportFormat format);
QList<Panel> sortPanelsForExport(const QList<Panel>& panels);
QString panelBaseName(const Panel& panel, int index);
QString panelDisplayIndex(const Panel& panel);
QString exportPanelHeaderText(const QString& novelTitle, int chapterNumber, int panelNumber);
QString sanitizeExportText(const QString& text);

}

#endif // EXPORTUTILS_H
