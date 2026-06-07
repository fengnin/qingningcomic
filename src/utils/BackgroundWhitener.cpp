#include "utils/BackgroundWhitener.h"

#include <QBuffer>
#include <QDebug>
#include <QIODevice>
#include <QQueue>
#include <QVector>
#include <algorithm>

namespace {
    constexpr int FLOOD_TOLERANCE_SQ = 15 * 15;

    int colorDistanceSq(QRgb a, QRgb b)
    {
        const int dr = qRed(a)   - qRed(b);
        const int dg = qGreen(a) - qGreen(b);
        const int db = qBlue(a)  - qBlue(b);
        return dr * dr + dg * dg + db * db;
    }

    // Sample a small border region around each corner and return the median color.
    // More robust than a single pixel when the corner has anti-aliasing or noise.
    QRgb sampleCornerColor(const QImage& image, int cornerX, int cornerY)
    {
        const int w = image.width();
        const int h = image.height();
        constexpr int SAMPLE_RADIUS = 4;

        const int x0 = qBound(0, cornerX - SAMPLE_RADIUS, w - 1);
        const int x1 = qBound(0, cornerX + SAMPLE_RADIUS, w - 1);
        const int y0 = qBound(0, cornerY - SAMPLE_RADIUS, h - 1);
        const int y1 = qBound(0, cornerY + SAMPLE_RADIUS, h - 1);

        QVector<int> reds, greens, blues;
        for (int y = y0; y <= y1; ++y) {
            const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
            for (int x = x0; x <= x1; ++x) {
                reds   << qRed(line[x]);
                greens << qGreen(line[x]);
                blues  << qBlue(line[x]);
            }
        }

        std::sort(reds.begin(),   reds.end());
        std::sort(greens.begin(), greens.end());
        std::sort(blues.begin(),  blues.end());

        const int mid = reds.size() / 2;
        return qRgb(reds[mid], greens[mid], blues[mid]);
    }

    int floodFillBackground(QImage& image)
    {
        const int width  = image.width();
        const int height = image.height();

        QVector<bool> visited(width * height, false);
        QQueue<int> queue;

        struct Corner { int x; int y; };
        const Corner corners[] = {
            {0,         0         },
            {width - 1, 0         },
            {0,         height - 1},
            {width - 1, height - 1}
        };

        // 动态容差：种子若偏离白色（avg<240），背景有色偏，需要更激进地清除
        int dynamicToleranceSq = FLOOD_TOLERANCE_SQ;
        bool hasColorCast = false;

        for (const Corner& c : corners) {
            const QRgb testSeed = sampleCornerColor(image, c.x, c.y);
            const int seedAvg = (qRed(testSeed) + qGreen(testSeed) + qBlue(testSeed)) / 3;

            if (seedAvg < 240) {
                hasColorCast = true;
                qDebug() << "BackgroundWhitener: 检测到背景色偏"
                         << "corner(" << c.x << "," << c.y << ") seedAvg=" << seedAvg;

                int deviation = 240 - seedAvg;
                int adaptiveTolerance = 15 + (deviation / 3);
                adaptiveTolerance = qMin(adaptiveTolerance, 30);
                dynamicToleranceSq = adaptiveTolerance * adaptiveTolerance;

                qDebug() << "BackgroundWhitener: 动态调整容差:"
                         << FLOOD_TOLERANCE_SQ << "->" << dynamicToleranceSq
                         << "(tolerance=" << adaptiveTolerance << ")";
                break;
            }
        }

        for (const Corner& c : corners) {
            const int idx = c.y * width + c.x;
            if (visited[idx]) continue;

            const QRgb seedColor = sampleCornerColor(image, c.x, c.y);
            qDebug() << "BackgroundWhitener: corner(" << c.x << "," << c.y << ")"
                     << "seedColor=rgb(" << qRed(seedColor) << qGreen(seedColor) << qBlue(seedColor) << ")"
                     << "toleranceSq=" << dynamicToleranceSq << (hasColorCast ? " [ADAPTIVE]" : "");
            visited[idx] = true;
            queue.enqueue(idx);

            int cornerCount = 0;
            while (!queue.isEmpty()) {
                const int cur = queue.dequeue();
                const int px  = cur % width;
                const int py  = cur / width;
                ++cornerCount;

                const int nx[] = {px - 1, px + 1, px,     px    };
                const int ny[] = {py,     py,      py - 1, py + 1};
                for (int i = 0; i < 4; ++i) {
                    const int x = nx[i], y = ny[i];
                    if (x < 0 || x >= width || y < 0 || y >= height) continue;
                    const int nidx = y * width + x;
                    if (visited[nidx]) continue;
                    if (colorDistanceSq(image.pixel(x, y), seedColor) <= dynamicToleranceSq) {
                        visited[nidx] = true;
                        queue.enqueue(nidx);
                    }
                }
            }
            qDebug() << "BackgroundWhitener: corner(" << c.x << "," << c.y << ") filled" << cornerCount << "pixels";
        }

        int count = 0;
        for (int y = 0; y < height; ++y) {
            QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
            for (int x = 0; x < width; ++x) {
                if (visited[y * width + x]) {
                    line[x] = qRgb(255, 255, 255);
                    ++count;
                }
            }
        }
        return count;
    }

    bool isEdgePixel(int x, int y, int width, int height, int margin)
    {
        return x < margin || x >= width - margin ||
               y < margin || y >= height - margin;
    }

    void logBackgroundQuality(const BackgroundWhitener::BackgroundQuality& quality)
    {
        if (quality.impurityRatio > 10.0) {
            qWarning() << "BackgroundWhitener: 背景纯度不达标，非白像素占比:"
                       << QString::number(quality.impurityRatio, 'f', 1) << "%"
                       << "最大偏差:" << quality.maxDeviation;
        } else if (quality.impurityRatio > 3.0) {
            qDebug() << "BackgroundWhitener: 背景有轻微杂质:"
                     << QString::number(quality.impurityRatio, 'f', 1) << "%";
        } else {
            qDebug() << "BackgroundWhitener: 背景纯度达标:"
                     << QString::number(quality.impurityRatio, 'f', 2) << "%";
        }
    }
}

QImage BackgroundWhitener::fillWhiteBackground(const QImage& image, int /*whiteThreshold*/)
{
    if (image.isNull()) {
        return image;
    }

    QImage result = image.convertToFormat(QImage::Format_ARGB32);

    const int floodCount = floodFillBackground(result);
    qDebug() << "BackgroundWhitener: Flood-fill converted" << floodCount << "pixels to white";

    validateBackgroundPurity(result);

    return result;
}

// 验证背景纯度，输出警告信息（不修改图像）
BackgroundWhitener::BackgroundQuality BackgroundWhitener::validateBackgroundPurity(const QImage& image)
{
    BackgroundQuality quality;
    const int width = image.width();
    const int height = image.height();

    const int margin = qMax(width, height) * 5 / 100;
    int sampleCount = 0;
    int impureCount = 0;
    int maxDeviation = 0;

    for (int y = 0; y < height; ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < width; ++x) {
            if (!isEdgePixel(x, y, width, height, margin)) continue;

            const QRgb pixel = line[x];
            const int avg = (qRed(pixel) + qGreen(pixel) + qBlue(pixel)) / 3;

            sampleCount++;

            if (avg < 250) {
                impureCount++;
                int deviation = 250 - avg;
                if (deviation > maxDeviation) {
                    maxDeviation = deviation;
                }
            }
        }
    }

    if (sampleCount > 0) {
        quality.impurityRatio = static_cast<double>(impureCount) / sampleCount * 100.0;
        quality.maxDeviation = maxDeviation;
        logBackgroundQuality(quality);
    }

    return quality;
}

QByteArray BackgroundWhitener::fillWhiteBackgroundImageData(const QByteArray& imageData, int whiteThreshold)
{
    if (imageData.isEmpty()) {
        return imageData;
    }

    QImage image;
    if (!image.loadFromData(imageData)) {
        qWarning() << "BackgroundWhitener: Failed to load image from data";
        return imageData;
    }

    const QImage processedImage = fillWhiteBackground(image, whiteThreshold);
    if (processedImage.isNull()) {
        return imageData;
    }

    QByteArray bufferData;
    QBuffer buffer(&bufferData);
    if (!buffer.open(QIODevice::WriteOnly)) {
        qWarning() << "BackgroundWhitener: Failed to open buffer";
        return imageData;
    }

    if (!processedImage.save(&buffer, "PNG")) {
        qWarning() << "BackgroundWhitener: Failed to save processed image";
        return imageData;
    }

    return bufferData;
}
