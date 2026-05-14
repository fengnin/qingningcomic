#pragma once

#include <QByteArray>
#include <QImage>

namespace BackgroundWhitener
{
    QImage fillWhiteBackground(const QImage& image, int whiteThreshold = 240);

    QByteArray fillWhiteBackgroundImageData(const QByteArray& imageData, int whiteThreshold = 240);

    void validateBackgroundPurity(const QImage& image);
}
