#include "utils/BackgroundWhitener.h"

#include <QBuffer>
#include <QDebug>
#include <QIODevice>
#include <QQueue>
#include <QVector>
#include <algorithm>

namespace {
    // Tighter tolerance: reduces bleed into light-colored clothing/hair.
    // Background pixels are typically very close to the corner seed color.
    constexpr int FLOOD_TOLERANCE_SQ = 18 * 18;

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

        // Four corners: sample median color from a small patch around each
        struct Corner { int x; int y; };
        const Corner corners[] = {
            {0,         0         },
            {width - 1, 0         },
            {0,         height - 1},
            {width - 1, height - 1}
        };

        for (const Corner& c : corners) {
            const int idx = c.y * width + c.x;
            if (visited[idx]) continue;

            const QRgb seedColor = sampleCornerColor(image, c.x, c.y);
            visited[idx] = true;
            queue.enqueue(idx);

            while (!queue.isEmpty()) {
                const int cur = queue.dequeue();
                const int px  = cur % width;
                const int py  = cur / width;

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
