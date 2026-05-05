#pragma once

#include <QByteArray>
#include <QImage>

class ImageBorderTrimmer
{
public:
    static QImage trimDarkBorder(const QImage& image, bool* trimmed = nullptr);
    static QByteArray trimDarkBorderImageData(const QByteArray& imageData, bool* trimmed = nullptr);
};
