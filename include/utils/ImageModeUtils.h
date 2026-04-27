#pragma once

#include "services/ImageService.h"
#include <QString>

namespace ImageModeUtils {

bool isPresetModeString(const QString& mode);
ImageService::GenerateMode generateModeFromString(const QString& mode);
ImageService::BatchPresetMode presetModeFromString(const QString& mode);
QString generateModeString(ImageService::GenerateMode mode);
QString presetModeString(ImageService::BatchPresetMode presetMode);

} // namespace ImageModeUtils
