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

    int floodFillBackground(QImage& image, QVector<bool>* externalVisited = nullptr)
    {
        const int width  = image.width();
        const int height = image.height();

        QVector<bool> localVisited;
        if (!externalVisited) {
            localVisited.fill(false, width * height);
            externalVisited = &localVisited;
        }
        QVector<bool>& visited = *externalVisited;
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

    // 第一步：清除边缘区域内亮度低于阈值的像素，打通矩形边框屏障
    int edgeBrightnessCleanup(QImage& image, int marginPercent = 8, int valueThreshold = 200)
    {
        const int width  = image.width();
        const int height = image.height();
        const int margin = qMax(width, height) * marginPercent / 100;

        int count = 0;
        auto clearIfDark = [&](int x, int y) {
            QRgb* px = reinterpret_cast<QRgb*>(image.scanLine(y)) + x;
            const int v = qMax(qRed(*px), qMax(qGreen(*px), qBlue(*px)));
            if (v < valueThreshold) { *px = qRgb(255, 255, 255); ++count; }
        };

        // 只扫描四条边缘带，不遍历全图内部
        for (int y = 0; y < margin; ++y)
            for (int x = 0; x < width; ++x) clearIfDark(x, y);           // 上
        for (int y = height - margin; y < height; ++y)
            for (int x = 0; x < width; ++x) clearIfDark(x, y);           // 下
        for (int y = margin; y < height - margin; ++y) {
            for (int x = 0; x < margin; ++x) clearIfDark(x, y);          // 左
            for (int x = width - margin; x < width; ++x) clearIfDark(x, y); // 右
        }
        return count;
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

    // 第一轮：flood-fill 清除外层背景
    QVector<bool> visited(result.width() * result.height(), false);
    const int floodCount1 = floodFillBackground(result, &visited);
    qDebug() << "BackgroundWhitener: 第一轮 flood-fill 转白" << floodCount1 << "像素";

    // 第二步：边缘亮度清除，打通模型生成的矩形边框屏障
    // margin 3% (~40px for 1328px)：足够覆盖边框线，不会触及人物头顶/脚踝
    const int edgeCount = edgeBrightnessCleanup(result, 3);
    qDebug() << "BackgroundWhitener: 边缘亮度清除转白" << edgeCount << "像素";

    // 第三步：用全新 visited 数组再次 flood-fill，使边框打通后的白色路径能被角点扩散利用
    // 不能复用旧 visited：四个角点已被标记，队列永不入队，第二轮会完全跳过
    const int floodCount2 = floodFillBackground(result);
    qDebug() << "BackgroundWhitener: 第二轮 flood-fill 转白" << floodCount2 << "像素";

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
