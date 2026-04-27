#include "utils/ImageModeUtils.h"

namespace ImageModeUtils {

bool isPresetModeString(const QString& mode)
{
    return mode == QLatin1String("1x1") || mode == QLatin1String("1:1")
        || mode == QLatin1String("3x2") || mode == QLatin1String("3:2")
        || mode == QLatin1String("16x9") || mode == QLatin1String("16:9");
}

ImageService::GenerateMode generateModeFromString(const QString& mode)
{
    return (mode == QLatin1String("hd")) ? ImageService::GenerateMode::HD : ImageService::GenerateMode::Preview;
}

ImageService::BatchPresetMode presetModeFromString(const QString& mode)
{
    if (mode == QLatin1String("3x2") || mode == QLatin1String("3:2")) {
        return ImageService::BatchPresetMode::Standard_3x2;
    }
    if (mode == QLatin1String("16x9") || mode == QLatin1String("16:9")) {
        return ImageService::BatchPresetMode::Widescreen_16x9;
    }
    return ImageService::BatchPresetMode::Square_1x1;
}

QString generateModeString(ImageService::GenerateMode mode)
{
    return (mode == ImageService::GenerateMode::HD) ? QStringLiteral("hd") : QStringLiteral("preview");
}

QString presetModeString(ImageService::BatchPresetMode presetMode)
{
    switch (presetMode) {
        case ImageService::BatchPresetMode::Square_1x1:
            return QStringLiteral("1x1");
        case ImageService::BatchPresetMode::Standard_3x2:
            return QStringLiteral("3x2");
        case ImageService::BatchPresetMode::Widescreen_16x9:
            return QStringLiteral("16x9");
    }
    return QStringLiteral("1x1");
}

} // namespace ImageModeUtils
