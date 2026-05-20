#ifndef CHARACTEREDITPROMPTBUILDER_H
#define CHARACTEREDITPROMPTBUILDER_H

#include <QString>
#include <QList>

struct CharacterFieldDiff;

class CharacterEditPromptBuilder
{
public:
    static QString buildFromDiff(const QList<CharacterFieldDiff>& diff);
};

#endif // CHARACTEREDITPROMPTBUILDER_H
