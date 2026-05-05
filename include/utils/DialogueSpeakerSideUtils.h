#pragma once

#include <QString>

class DialogueSpeakerSideUtils
{
public:
    static QString normalize(const QString& side);
    static QString marker(const QString& side);
    static QString labelWithSide(const QString& speaker, const QString& side);
    static void splitLabelAndSide(QString& speaker, QString& side);
};
