#include "utils/ImageColorUtils.h"

int ImageColorUtils::luminance(const QColor& color)
{
    return static_cast<int>(0.299 * color.red() + 0.587 * color.green() + 0.114 * color.blue());
}
