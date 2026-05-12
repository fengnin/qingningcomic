#include "utils/BackgroundWhitener.h"

#include <QBuffer>
#include <QDebug>
#include <QIODevice>
#include <QQueue>
#include <QVector>

namespace {
    // Max color distance from the corner seed to be considered background.
    // Kept moderate: large enough to handle gradients/anti-aliasing,
    // small enough to stop at character boundaries.
    constexpr int FLOOD_TOLERANCE_SQ = 30 * 30; // compare squared distance to avoid sqrt

    int colorDistanceSq(QRgb a, QRgb b)
    {
        const int dr = qRed(a)   - qRed(b);
        const int dg = qGreen(a) - qGreen(b);
        const int db = qBlue(a)  - qBlue(b);
        return dr * dr + dg * dg + db * db;
    }

    // Flood-fill from all 4 corners to find and whiten background pixels.
    // Each corner seeds its own fill using that corner's pixel color.
    // Only connected regions reachable from a corner are affected, so the
    // character body is never touched regardless of its color.
    int floodFillBackground(QImage& image)
    {
        const int width = image.width();
        const int height = image.height();

        QVector<bool> visited(width * height, false);
        // Store flat index instead of QPoint to halve queue memory
        QQueue<int> queue;

        const int corners[] = {
            0,
            width - 1,
            (height - 1) * width,
            (height - 1) * width + width - 1
        };

        for (int cornerIdx : corners) {
            if (visited[cornerIdx]) continue;

            const QRgb seedColor = image.pixel(cornerIdx % width, cornerIdx / width);
            visited[cornerIdx] = true;
            queue.enqueue(cornerIdx);

            while (!queue.isEmpty()) {
                const int idx = queue.dequeue();
                const int px = idx % width;
                const int py = idx / width;

                const int nx[] = {px - 1, px + 1, px,     px    };
                const int ny[] = {py,     py,      py - 1, py + 1};
                for (int i = 0; i < 4; ++i) {
                    const int x = nx[i], y = ny[i];
                    if (x < 0 || x >= width || y < 0 || y >= height) continue;
                    const int nidx = y * width + x;
                    if (visited[nidx]) continue;
                    if (colorDistanceSq(image.pixel(x, y), seedColor) <= FLOOD_TOLERANCE_SQ) {
                        visited[nidx] = true;
                        queue.enqueue(nidx);
                    }
                }
            }
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
}

QImage BackgroundWhitener::fillWhiteBackground(const QImage& image, int /*whiteThreshold*/)
{
    if (image.isNull()) {
        return image;
    }

    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    const int count = floodFillBackground(result);
    qDebug() << "BackgroundWhitener: Flood-fill converted" << count << "pixels to white";
    return result;
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
