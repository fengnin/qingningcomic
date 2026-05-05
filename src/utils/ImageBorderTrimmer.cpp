#include "utils/ImageBorderTrimmer.h"

#include <QBuffer>
#include <QColor>
#include <QIODevice>
#include <QRect>
#include <QtMath>

namespace {
    constexpr int kEdgeSampleCount = 4;
    constexpr int kBlackThreshold = 42;
    constexpr int kDarkBorderMinRatio = 3;

    int luminance(const QColor& color)
    {
        return static_cast<int>(0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue());
    }

    bool isDarkPixel(const QColor& color)
    {
        return luminance(color) <= kBlackThreshold;
    }

    bool rowLooksLikeBorder(const QImage& image, int y)
    {
        if (y < 0 || y >= image.height()) {
            return false;
        }

        int darkCount = 0;
        const int samples[] = {0, image.width() / 3, (image.width() * 2) / 3, qMax(0, image.width() - 1)};
        for (int x : samples) {
            if (x >= 0 && x < image.width() && isDarkPixel(image.pixelColor(x, y))) {
                ++darkCount;
            }
        }
        return darkCount >= kDarkBorderMinRatio;
    }

    bool columnLooksLikeBorder(const QImage& image, int x)
    {
        if (x < 0 || x >= image.width()) {
            return false;
        }

        int darkCount = 0;
        const int samples[] = {0, image.height() / 3, (image.height() * 2) / 3, qMax(0, image.height() - 1)};
        for (int y : samples) {
            if (y >= 0 && y < image.height() && isDarkPixel(image.pixelColor(x, y))) {
                ++darkCount;
            }
        }
        return darkCount >= kDarkBorderMinRatio;
    }

    int detectTopTrim(const QImage& image)
    {
        const int maxScan = qMax(1, image.height() / 2);
        int trim = 0;
        for (int y = 0; y < qMin(maxScan, image.height()); ++y) {
            if (!rowLooksLikeBorder(image, y)) {
                break;
            }
            trim = y + 1;
        }
        return trim;
    }

    int detectBottomTrim(const QImage& image)
    {
        const int maxScan = qMax(1, image.height() / 2);
        int trim = 0;
        for (int offset = 0; offset < qMin(maxScan, image.height()); ++offset) {
            const int y = image.height() - 1 - offset;
            if (!rowLooksLikeBorder(image, y)) {
                break;
            }
            trim = offset + 1;
        }
        return trim;
    }

    int detectLeftTrim(const QImage& image)
    {
        const int maxScan = qMax(1, image.width() / 2);
        int trim = 0;
        for (int x = 0; x < qMin(maxScan, image.width()); ++x) {
            if (!columnLooksLikeBorder(image, x)) {
                break;
            }
            trim = x + 1;
        }
        return trim;
    }

    int detectRightTrim(const QImage& image)
    {
        const int maxScan = qMax(1, image.width() / 2);
        int trim = 0;
        for (int offset = 0; offset < qMin(maxScan, image.width()); ++offset) {
            const int x = image.width() - 1 - offset;
            if (!columnLooksLikeBorder(image, x)) {
                break;
            }
            trim = offset + 1;
        }
        return trim;
    }
}

QImage ImageBorderTrimmer::trimDarkBorder(const QImage& image, bool* trimmed)
{
    if (trimmed) {
        *trimmed = false;
    }

    if (image.isNull() || image.width() < 8 || image.height() < 8) {
        return image;
    }

    const int left = detectLeftTrim(image);
    const int top = detectTopTrim(image);
    const int right = detectRightTrim(image);
    const int bottom = detectBottomTrim(image);

    if (left <= 0 && top <= 0 && right <= 0 && bottom <= 0) {
        return image;
    }

    const QRect cropRect(left,
                         top,
                         qMax(1, image.width() - left - right),
                         qMax(1, image.height() - top - bottom));
    if (!cropRect.isValid() || cropRect.width() <= 1 || cropRect.height() <= 1) {
        return image;
    }

    if (trimmed) {
        *trimmed = true;
    }
    return image.copy(cropRect);
}

QByteArray ImageBorderTrimmer::trimDarkBorderImageData(const QByteArray& imageData, bool* trimmed)
{
    if (trimmed) {
        *trimmed = false;
    }

    if (imageData.isEmpty()) {
        return imageData;
    }

    QImage image;
    if (!image.loadFromData(imageData)) {
        return imageData;
    }

    bool didTrim = false;
    const QImage trimmedImage = trimDarkBorder(image, &didTrim);
    if (!didTrim || trimmedImage.isNull() || trimmedImage.size() == image.size()) {
        return imageData;
    }

    QByteArray bufferData;
    QBuffer buffer(&bufferData);
    if (!buffer.open(QIODevice::WriteOnly)) {
        return imageData;
    }
    if (!trimmedImage.save(&buffer, "PNG")) {
        return imageData;
    }

    if (trimmed) {
        *trimmed = true;
    }
    return bufferData;
}
