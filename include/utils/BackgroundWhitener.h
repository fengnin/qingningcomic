#pragma once

#include <QByteArray>
#include <QImage>

namespace BackgroundWhitener
{
    struct BackgroundQuality {
        double impurityRatio = 0.0;
        int maxDeviation = 0;

        bool isAcceptable(double threshold = 20.0) const
        {
            return impurityRatio <= threshold;
        }
    };

    QImage fillWhiteBackground(const QImage& image, int whiteThreshold = 240);

    QByteArray fillWhiteBackgroundImageData(const QByteArray& imageData, int whiteThreshold = 240);

    BackgroundQuality validateBackgroundPurity(const QImage& image);
}
