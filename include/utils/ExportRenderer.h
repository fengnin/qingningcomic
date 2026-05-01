#ifndef EXPORTRENDERER_H
#define EXPORTRENDERER_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QMap>

class Novel;
class Storyboard;
class Panel;
class QImage;

namespace ExportRenderer {

struct ExportAsset
{
    QByteArray data;
    QString fileName;
    QString contentType;
};

QImage renderPanelCard(const Novel& novel, const Storyboard& storyboard, const Panel& panel, int scale = 2);
QByteArray exportPanelsToPdf(const Novel& novel, const Storyboard& storyboard, const QList<Panel>& panels);
QByteArray exportPanelsToWebtoon(const QList<Panel>& panels);
QByteArray exportPanelsToResourcesZip(const Novel& novel, const Storyboard& storyboard, const QList<Panel>& panels);
bool writeExportFile(const QString& exportPath, const QByteArray& data);

}

#endif // EXPORTRENDERER_H
