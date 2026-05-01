#include "utils/ExportRenderer.h"
#include "utils/ExportUtils.h"
#include "models/Novel.h"
#include "models/Storyboard.h"
#include "models/Panel.h"
#include "api/SharedNetworkManager.h"
#include "data/FileStorage.h"
#include "utils/Logger.h"

#include <QBuffer>
#include <QColor>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QFont>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QPainterPath>
#include <QScopedPointer>
#include <QPainter>
#include <QEventLoop>
#include <QPdfWriter>
#include <QPageSize>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTime>
#include <QtMath>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTextDocument>
#include <QVariantMap>

namespace {
constexpr int kPdfPageWidth = 1240;
constexpr int kPdfPageHeight = 1754;
constexpr int kPdfMargin = 72;
constexpr int kCardWidth = 1240;
constexpr int kCardHeight = 1500;

struct DosTimeDate
{
    quint16 time;
    quint16 date;
};

struct BufferRecord
{
    QByteArray name;
    quint32 crc = 0;
    int size = 0;
    int offset = 0;
    quint16 time = 0;
    quint16 date = 0;
};

void writeLE16(QByteArray& buffer, int offset, quint16 value)
{
    buffer[offset] = static_cast<char>(value & 0xff);
    buffer[offset + 1] = static_cast<char>((value >> 8) & 0xff);
}

void writeLE32(QByteArray& buffer, int offset, quint32 value)
{
    buffer[offset] = static_cast<char>(value & 0xff);
    buffer[offset + 1] = static_cast<char>((value >> 8) & 0xff);
    buffer[offset + 2] = static_cast<char>((value >> 16) & 0xff);
    buffer[offset + 3] = static_cast<char>((value >> 24) & 0xff);
}

DosTimeDate dateToDos(const QDateTime& dt)
{
    const QDate date = dt.date();
    const QTime time = dt.time();
    DosTimeDate result;
    result.date = static_cast<quint16>(((date.year() - 1980) << 9) | (date.month() << 5) | date.day());
    result.time = static_cast<quint16>((time.hour() << 11) | (time.minute() << 5) | (time.second() / 2));
    return result;
}

quint32 crc32(const QByteArray& data)
{
    static quint32 table[256];
    static bool tableInitialized = false;
    if (!tableInitialized) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int j = 0; j < 8; ++j) {
                c = (c & 1) ? (0xEDB88320U ^ (c >> 1)) : (c >> 1);
            }
            table[i] = c;
        }
        tableInitialized = true;
    }

    quint32 crc = 0xFFFFFFFFU;
    for (const unsigned char ch : data) {
        crc = table[(crc ^ ch) & 0xFFU] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFU;
}

QString resolveLocalImagePath(const QString& path)
{
    if (path.isEmpty()) {
        return QString();
    }
    if (QFileInfo(path).exists()) {
        return path;
    }
    if (FileStorage::instance()) {
        const QString fullPath = FileStorage::instance()->getFullPath(path);
        if (QFileInfo(fullPath).exists()) {
            return fullPath;
        }
    }
    return path;
}

QByteArray downloadRemoteImageBytes(const QString& url)
{
    if (url.isEmpty()) {
        return QByteArray();
    }

    QNetworkRequest request{QUrl(url)};
    QNetworkReply* reply = SharedNetworkManager::instance()->manager()->get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data;
    if (reply->error() == QNetworkReply::NoError) {
        data = reply->readAll();
    } else {
        LOG_WARNING("ExportRenderer", QString("Failed to download image from url: %1, error: %2")
            .arg(url.left(120), reply->errorString()));
    }
    reply->deleteLater();
    return data;
}

QImage loadPanelImage(const Panel& panel)
{
    auto loadFromPathOrUrl = [](const QString& rawPath) -> QImage {
        const QString path = resolveLocalImagePath(rawPath);
        if (path.isEmpty()) {
            return QImage();
        }

        QImage image(path);
        if (!image.isNull()) {
            return image;
        }

        if (path.startsWith("http://") || path.startsWith("https://")) {
            const QByteArray bytes = downloadRemoteImageBytes(path);
            QImage remoteImage;
            if (!bytes.isEmpty() && remoteImage.loadFromData(bytes)) {
                return remoteImage;
            }
        }
        return QImage();
    };

    QImage image = loadFromPathOrUrl(panel.hdUrl());
    if (!image.isNull()) {
        return image;
    }

    image = loadFromPathOrUrl(panel.previewUrl());
    if (!image.isNull()) {
        return image;
    }

    QImage fallback(1024, 768, QImage::Format_ARGB32_Premultiplied);
    fallback.fill(QColor("#f3f4f6"));
    return fallback;
}

void drawRoundedImage(QPainter& painter, const QRectF& target, const QImage& source)
{
    if (source.isNull() || target.isEmpty()) {
        return;
    }

    QImage scaled = source.scaled(target.size().toSize(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    const QPointF topLeft(target.x() + (target.width() - scaled.width()) / 2.0,
                          target.y() + (target.height() - scaled.height()) / 2.0);

    QPainterPath clip;
    clip.addRoundedRect(target, 24, 24);
    painter.save();
    painter.setClipPath(clip);
    painter.fillRect(target, QColor("#ffffff"));
    painter.drawImage(topLeft, scaled);
    painter.restore();
}

void drawLabel(QPainter& painter, const QRectF& rect, const QString& text, int pointSize, bool bold = false)
{
    QFont font(QStringLiteral("Microsoft YaHei"), pointSize, bold ? QFont::Bold : QFont::Normal);
    painter.setFont(font);
    painter.setPen(QColor("#111827"));
    painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, text);
}

QByteArray imageToPngBytes(const QImage& image)
{
    QByteArray bufferData;
    QBuffer buffer(&bufferData);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return QByteArray();
    }
    if (!image.save(&buffer, "PNG")) {
        return QByteArray();
    }
    return bufferData;
}

QImage createFallbackExportImage()
{
    QImage fallback(1200, 800, QImage::Format_ARGB32_Premultiplied);
    fallback.fill(Qt::white);
    return fallback;
}

QJsonObject panelToJson(const Panel& panel);

void drawPdfHeader(QPainter& painter, const Novel& novel)
{
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 24, QFont::Bold));
    const QString title = novel.title().isEmpty() ? QStringLiteral("未命名作品") : novel.title();
    painter.drawText(QRectF(100, 120, 700, 80), ExportUtils::sanitizeExportText(title));
}

void drawPanelInfoField(QPainter& painter, const QRectF& titleRect, const QRectF& valueRect,
                        const QString& title, const QString& value, int valueFontSize)
{
    drawLabel(painter, titleRect, title, 16, true);
    drawLabel(painter, valueRect, ExportUtils::sanitizeExportText(value.isEmpty() ? QStringLiteral("—") : value), valueFontSize);
}

qreal measureWrappedTextHeight(const QString& text, qreal width, int pointSize)
{
    if (text.isEmpty() || width <= 0) {
        return 0;
    }

    QTextDocument doc;
    doc.setDefaultFont(QFont(QStringLiteral("Microsoft YaHei"), pointSize));
    doc.setDocumentMargin(0);
    doc.setPlainText(ExportUtils::sanitizeExportText(text));
    doc.setTextWidth(width);
    return doc.size().height();
}

qreal drawPanelCardInfoField(QPainter& painter, qreal x, qreal y, qreal width,
                             const QString& title, const QString& value, int valueFontSize)
{
    const qreal titleHeight = 28;
    const qreal valueGap = 34;
    const QString sanitizedValue = ExportUtils::sanitizeExportText(value.isEmpty() ? QStringLiteral("—") : value);
    const qreal minValueHeight = valueFontSize * 1.8;
    const qreal measuredValueHeight = qMax(minValueHeight, measureWrappedTextHeight(sanitizedValue, width, valueFontSize));

    drawPanelInfoField(painter, QRectF(x, y, 300, titleHeight), QRectF(x, y + valueGap, width, measuredValueHeight),
                       title, sanitizedValue, valueFontSize);
    return valueGap + measuredValueHeight;
}

qreal measurePanelCardInfoHeight(const Panel& panel, qreal width)
{
    const qreal innerWidth = qMax<qreal>(width - 64, 0);
    qreal total = 24;
    total += 34 + qMax<qreal>(14 * 1.8, measureWrappedTextHeight(panel.scene(), innerWidth, 14));
    total += 18;
    total += 34 + qMax<qreal>(14 * 1.8, measureWrappedTextHeight(QStringLiteral("%1 / %2").arg(panel.shotType(), panel.cameraAngle()), innerWidth, 14));
    total += 16;
    total += 34 + qMax<qreal>(14 * 1.8, measureWrappedTextHeight(panel.charactersText(), innerWidth, 14));
    total += 16;
    total += 34 + qMax<qreal>(13 * 1.8, measureWrappedTextHeight(panel.dialogueText(), innerWidth, 13));
    total += 24;
    return total;
}

qreal measurePanelCardBodyHeight(const Panel& panel, qreal cardWidth)
{
    const qreal contentWidth = cardWidth - 104;
    const qreal infoHeight = measurePanelCardInfoHeight(panel, contentWidth);
    const qreal imageHeight = 700;
    const qreal imageTop = 240;
    const qreal imageBottom = imageTop + imageHeight;
    const qreal infoTop = qMax(imageBottom + 24, 920.0);
    return infoTop + infoHeight + 40;
}

void drawPanelCardInfoSection(QPainter& painter, const Panel& panel, const QRectF& infoRect)
{
    const qreal left = 84;
    const qreal width = infoRect.width() - 64;
    qreal y = infoRect.top() + 24;

    y += drawPanelCardInfoField(painter, left, y, width, QStringLiteral("场景"), panel.scene(), 14);
    y += 18;
    y += drawPanelCardInfoField(painter, left, y, width, QStringLiteral("景别 / 机位"),
                                QStringLiteral("%1 / %2").arg(panel.shotType(), panel.cameraAngle()), 14);
    y += 16;
    y += drawPanelCardInfoField(painter, left, y, width, QStringLiteral("角色"), panel.charactersText(), 14);
    y += 16;
    drawPanelCardInfoField(painter, left, y, width, QStringLiteral("对白"), panel.dialogueText(), 13);
}

void drawPdfPage(QPainter& painter, QPdfWriter& writer, const Novel& novel, const Storyboard& storyboard,
                 const Panel& panel, bool firstPage)
{
    if (!firstPage) {
        writer.newPage();
    }

    drawPdfHeader(painter, novel);

    const QImage card = ExportRenderer::renderPanelCard(novel, storyboard, panel, 1);
    const QImage scaled = card.scaled(1000, 1300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    painter.drawImage(QRectF(100, 240, 1000, 1300), scaled);
}

QList<QPair<QString, QByteArray>> buildExportEntries(const Novel& novel, const Storyboard& storyboard, const QList<Panel>& panels)
{
    QList<QPair<QString, QByteArray>> entries;
    entries.reserve(panels.size() * 2 + 4);
    for (int i = 0; i < panels.size(); ++i) {
        const Panel& panel = panels.at(i);
        const QString baseName = ExportUtils::panelBaseName(panel, i);

        entries.append(qMakePair(
            QStringLiteral("panels/%1.json").arg(baseName),
            QJsonDocument(panelToJson(panel)).toJson(QJsonDocument::Indented)));

        entries.append(qMakePair(
            QStringLiteral("images/%1.png").arg(baseName),
            imageToPngBytes(loadPanelImage(panel))));
    }

    const QDateTime now = QDateTime::currentDateTime();
    const QJsonObject metadata{
        {"novel", QJsonObject{
            {"id", novel.id()},
            {"title", novel.title()}
        }},
        {"storyboard", QJsonObject{
            {"id", storyboard.id()},
            {"chapterNumber", storyboard.chapterNumber()},
            {"totalPages", storyboard.totalPages()},
            {"panelCount", storyboard.panelCount()},
            {"version", storyboard.version()}
        }},
        {"panelCount", panels.size()},
        {"generatedAt", now.toString(Qt::ISODate)}
    };
    entries.append(qMakePair(QStringLiteral("metadata.json"), QJsonDocument(metadata).toJson(QJsonDocument::Indented)));
    entries.append(qMakePair(
        QStringLiteral("novel.json"),
        QJsonDocument::fromVariant(novel.toVariantMap()).toJson(QJsonDocument::Indented)));

    QVariantMap storyboardMap;
    storyboardMap[QStringLiteral("id")] = storyboard.id();
    storyboardMap[QStringLiteral("novelId")] = storyboard.novelId();
    storyboardMap[QStringLiteral("chapterNumber")] = storyboard.chapterNumber();
    storyboardMap[QStringLiteral("totalPages")] = storyboard.totalPages();
    storyboardMap[QStringLiteral("panelCount")] = storyboard.panelCount();
    storyboardMap[QStringLiteral("version")] = storyboard.version();
    entries.append(qMakePair(
        QStringLiteral("storyboard.json"),
        QJsonDocument::fromVariant(storyboardMap).toJson(QJsonDocument::Indented)));
    return entries;
}

QByteArray createZipBuffer(const QList<QPair<QString, QByteArray>>& entries)
{
    QByteArray output;
    QVector<BufferRecord> centralRecords;
    QVector<QByteArray> parts;
    int offset = 0;

    for (const auto& entry : entries) {
        const QByteArray name = entry.first.toUtf8();
        const QByteArray data = entry.second;
        const quint32 crc = crc32(data);
        const auto dos = dateToDos(QDateTime::currentDateTime());

        QByteArray localHeader(30, '\0');
        writeLE32(localHeader, 0, 0x04034b50);
        writeLE16(localHeader, 4, 20);
        writeLE16(localHeader, 6, 0);
        writeLE16(localHeader, 8, 0);
        writeLE16(localHeader, 10, dos.time);
        writeLE16(localHeader, 12, dos.date);
        writeLE32(localHeader, 14, crc);
        writeLE32(localHeader, 18, data.size());
        writeLE32(localHeader, 22, data.size());
        writeLE16(localHeader, 26, name.size());
        writeLE16(localHeader, 28, 0);

        parts.append(localHeader);
        parts.append(name);
        parts.append(data);

        BufferRecord record;
        record.name = name;
        record.crc = crc;
        record.size = data.size();
        record.offset = offset;
        record.time = dos.time;
        record.date = dos.date;
        centralRecords.append(record);
        offset += localHeader.size() + name.size() + data.size();
    }

    QByteArray central;
    for (const BufferRecord& record : centralRecords) {
        QByteArray header(46, '\0');
        writeLE32(header, 0, 0x02014b50);
        writeLE16(header, 4, 20);
        writeLE16(header, 6, 20);
        writeLE16(header, 8, 0);
        writeLE16(header, 10, 0);
        writeLE16(header, 12, record.time);
        writeLE16(header, 14, record.date);
        writeLE32(header, 16, record.crc);
        writeLE32(header, 20, record.size);
        writeLE32(header, 24, record.size);
        writeLE16(header, 28, record.name.size());
        writeLE16(header, 30, 0);
        writeLE16(header, 32, 0);
        writeLE16(header, 34, 0);
        writeLE16(header, 36, 0);
        writeLE32(header, 38, 0);
        writeLE32(header, 42, record.offset);
        central.append(header);
        central.append(record.name);
    }

    QByteArray endRecord(22, '\0');
    writeLE32(endRecord, 0, 0x06054b50);
    writeLE16(endRecord, 4, 0);
    writeLE16(endRecord, 6, 0);
    writeLE16(endRecord, 8, centralRecords.size());
    writeLE16(endRecord, 10, centralRecords.size());
    writeLE32(endRecord, 12, central.size());
    writeLE32(endRecord, 16, offset);
    writeLE16(endRecord, 20, 0);

    for (const QByteArray& part : parts) {
        output.append(part);
    }
    output.append(central);
    output.append(endRecord);
    return output;
}

QByteArray readFileBytes(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    return file.readAll();
}

QJsonObject panelToJson(const Panel& panel)
{
    QJsonObject json;
    json["id"] = panel.id();
    json["storyboardId"] = panel.storyboardId();
    json["page"] = panel.page();
    json["index"] = panel.index();
    json["scene"] = panel.scene();
    json["shotType"] = panel.shotType();
    json["cameraAngle"] = panel.cameraAngle();
    json["visualPrompt"] = panel.visualPrompt();
    json["visualPromptEn"] = panel.visualPromptEn();
    json["status"] = panel.status();
    json["previewS3Key"] = panel.previewS3Key();
    json["hdS3Key"] = panel.hdS3Key();
    json["previewUrl"] = panel.previewUrl();
    json["hdUrl"] = panel.hdUrl();
    json["content"] = panel.content();
    return json;
}

}

namespace ExportRenderer {

QImage renderPanelCard(const Novel& novel, const Storyboard& storyboard, const Panel& panel, int scale)
{
    Q_UNUSED(storyboard)
    const int s = qMax(scale, 1);
    const int bodyHeight = static_cast<int>(qCeil(measurePanelCardBodyHeight(panel, kCardWidth)));
    QImage canvas(kCardWidth * s, bodyHeight * s, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(QColor("#f8fafc"));

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.scale(s, s);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor("#ffffff")));
    painter.drawRoundedRect(QRectF(24, 24, kCardWidth - 48, bodyHeight - 48), 28, 28);

    painter.setBrush(QBrush(QColor("#eef2ff")));
    painter.drawRoundedRect(QRectF(52, 52, 180, 52), 18, 18);
    painter.setPen(QColor("#4338ca"));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 18, QFont::Bold));
    painter.drawText(QRectF(68, 60, 160, 36), Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("高清成品"));

    painter.setPen(QColor("#0f172a"));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 30, QFont::Bold));
    painter.drawText(QRectF(52, 126, kCardWidth - 104, 56), Qt::AlignLeft | Qt::AlignVCenter,
                     novel.title().isEmpty() ? QStringLiteral("未命名作品") : novel.title());

    painter.setPen(QColor("#64748b"));
    painter.setFont(QFont(QStringLiteral("Microsoft YaHei"), 16));
    painter.drawText(QRectF(52, 188, kCardWidth - 104, 30), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("第 %1 章 · 面板 %2")
                         .arg(storyboard.chapterNumber())
                         .arg(ExportUtils::panelDisplayIndex(panel)));

    const QRectF imageRect(52, 240, kCardWidth - 104, 650);
    painter.setBrush(QColor("#e2e8f0"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(imageRect, 24, 24);
    drawRoundedImage(painter, imageRect, loadPanelImage(panel));

    const qreal infoTop = 920;
    const qreal infoHeight = measurePanelCardInfoHeight(panel, kCardWidth - 104);
    const QRectF infoRect(52, infoTop, kCardWidth - 104, infoHeight);
    painter.setBrush(QColor("#f8fafc"));
    painter.setPen(QPen(QColor("#e2e8f0"), 1));
    painter.drawRoundedRect(infoRect, 24, 24);
    drawPanelCardInfoSection(painter, panel, infoRect);

    painter.end();
    return canvas;
}

QByteArray exportPanelsToPdf(const Novel& novel, const Storyboard& storyboard, const QList<Panel>& panels)
{
    const QList<Panel> sortedPanels = ExportUtils::sortPanelsForExport(panels);
    QByteArray bytes;
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        return QByteArray();
    }
    tempFile.close();

    QPdfWriter writer(tempFile.fileName());
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setResolution(144);

    QPainter painter(&writer);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    for (int i = 0; i < sortedPanels.size(); ++i) {
        drawPdfPage(painter, writer, novel, storyboard, sortedPanels.at(i), i == 0);
    }

    painter.end();
    return readFileBytes(tempFile.fileName());
}

QByteArray exportPanelsToWebtoon(const QList<Panel>& panels)
{
    const QList<Panel> sortedPanels = ExportUtils::sortPanelsForExport(panels);
    QList<QImage> images;
    int width = 0;
    int height = 0;
    const int gutter = 32;

    for (const Panel& panel : sortedPanels) {
        QImage image = loadPanelImage(panel).convertToFormat(QImage::Format_ARGB32_Premultiplied);
        if (image.isNull()) {
            continue;
        }
        width = qMax(width, image.width());
        height += image.height();
        if (!images.isEmpty()) {
            height += gutter;
        }
        images.append(image);
    }

    if (images.isEmpty()) {
        images.append(createFallbackExportImage());
        width = images.first().width();
        height = images.first().height();
    }

    QImage canvas(width, height, QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::white);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    int y = 0;
    for (const QImage& image : images) {
        const int x = (width - image.width()) / 2;
        painter.drawImage(QPoint(x, y), image);
        y += image.height() + gutter;
    }
    painter.end();
    return imageToPngBytes(canvas);
}

QByteArray exportPanelsToResourcesZip(const Novel& novel, const Storyboard& storyboard, const QList<Panel>& panels)
{
    const QList<Panel> sortedPanels = ExportUtils::sortPanelsForExport(panels);
    return createZipBuffer(buildExportEntries(novel, storyboard, sortedPanels));
}

}
