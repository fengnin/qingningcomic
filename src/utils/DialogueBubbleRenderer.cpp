#include "utils/DialogueBubbleRenderer.h"
#include "utils/DialogueSpeakerSideUtils.h"
#include "utils/Logger.h"
#include <algorithm>
#include <QtMath>

namespace {
    QString pointToString(const QPointF& point)
    {
        return QStringLiteral("%1,%2")
            .arg(point.x(), 0, 'f', 1)
            .arg(point.y(), 0, 'f', 1);
    }

    QString rectToString(const QRectF& rect)
    {
        return QStringLiteral("%1,%2 %3x%4")
            .arg(rect.left(), 0, 'f', 1)
            .arg(rect.top(), 0, 'f', 1)
            .arg(rect.width(), 0, 'f', 1)
            .arg(rect.height(), 0, 'f', 1);
    }

    QString characterPositionToString(CharacterPosition position)
    {
        switch (position) {
        case CharacterPosition::Left:
            return QStringLiteral("Left");
        case CharacterPosition::Right:
            return QStringLiteral("Right");
        case CharacterPosition::Center:
            return QStringLiteral("Center");
        case CharacterPosition::Unknown:
        default:
            return QStringLiteral("Unknown");
        }
    }

    BubbleLayout makeBaseBubbleLayout(const DialogueLine& dialogue, const DialogueBubbleRenderer::Style& style)
    {
        BubbleLayout layout;
        layout.text = dialogue.text;
        layout.speaker = dialogue.speaker;
        layout.bubbleType = dialogue.bubbleType;
        layout.fontSize = style.fontSize;
        return layout;
    }

    QString normalizeSpeakerKey(const QString& text)
    {
        QString normalized;
        normalized.reserve(text.size());
        for (const QChar& ch : text.trimmed().toLower()) {
            if (ch.isSpace()) {
                continue;
            }

            const QChar::Category category = ch.category();
            if (category == QChar::Punctuation_Connector ||
                category == QChar::Punctuation_Dash ||
                category == QChar::Punctuation_Open ||
                category == QChar::Punctuation_Close ||
                category == QChar::Punctuation_Other ||
                category == QChar::Punctuation_InitialQuote ||
                category == QChar::Punctuation_FinalQuote) {
                continue;
            }

            normalized.append(ch);
        }
        return normalized;
    }

    QRectF alignBubbleRectToSpeaker(const QRectF& textRect, CharacterPosition charPos, const QPointF& facePosition)
    {
        qreal x = facePosition.x() - textRect.width() / 2.0;
        if (charPos == CharacterPosition::Left) {
            x = facePosition.x() - textRect.width() * 0.10;
        } else if (charPos == CharacterPosition::Right) {
            x = facePosition.x() - textRect.width() * 0.90;
        }
        return QRectF(x, 0.0, textRect.width(), textRect.height());
    }

    QRectF clampBubbleRectToImage(const QRectF& rect, int imageWidth, int imageHeight)
    {
        QRectF clamped = rect;
        if (clamped.width() > imageWidth) {
            clamped.setWidth(imageWidth);
        }
        if (clamped.height() > imageHeight) {
            clamped.setHeight(imageHeight);
        }

        const qreal maxX = qMax<qreal>(0.0, imageWidth - clamped.width());
        const qreal maxY = qMax<qreal>(0.0, imageHeight - clamped.height());
        clamped.moveLeft(qBound<qreal>(0.0, clamped.left(), maxX));
        clamped.moveTop(qBound<qreal>(0.0, clamped.top(), maxY));
        return clamped;
    }

    qreal rectCenterDistance(const QRectF& a, const QPointF& b)
    {
        const qreal dx = a.center().x() - b.x();
        const qreal dy = a.center().y() - b.y();
        return qSqrt(dx * dx + dy * dy);
    }

    QPointF bubbleTailBasePoint(const QRectF& rect, bool isLeft, qreal yRatio)
    {
        const qreal tailBaseY = rect.bottom() - rect.height() * yRatio;
        return isLeft ? QPointF(rect.left(), tailBaseY) : QPointF(rect.right(), tailBaseY);
    }

    QColor fillColorForBubbleType(const QString& bubbleType, const DialogueBubbleRenderer::Style& style)
    {
        if (bubbleType == QStringLiteral("thought")) {
            return style.thoughtFillColor;
        }
        if (bubbleType == QStringLiteral("narration")) {
            return style.narrationFillColor;
        }
        if (bubbleType == QStringLiteral("scream") || bubbleType == QStringLiteral("shout")) {
            return style.screamFillColor;
        }
        return style.fillColor;
    }

    void advanceBubbleColumn(qreal& columnY, const QRectF& rect, const DialogueBubbleRenderer::Style& style)
    {
        columnY = qMax(columnY, rect.bottom() + style.bubbleSpacing);
    }

    void drawTriangleTail(QPainter& painter,
                          const QRectF& rect,
                          const QPointF& target,
                          const DialogueBubbleRenderer::Style& style,
                          bool isLeft)
    {
        painter.save();
        painter.setBrush(fillColorForBubbleType(QStringLiteral("speech"), style));
        painter.setPen(QPen(style.strokeColor, style.strokeWidth));

        const QPointF baseCenter = bubbleTailBasePoint(rect, isLeft, 0.3);
        const QPointF baseTop = baseCenter + QPointF(0, -6);
        const QPointF baseBottom = baseCenter + QPointF(0, 6);

        QPainterPath tailPath;
        tailPath.moveTo(baseTop);
        tailPath.lineTo(target);
        tailPath.lineTo(baseBottom);
        tailPath.closeSubpath();
        painter.drawPath(tailPath);
        painter.restore();
    }

    void drawDotTail(QPainter& painter,
                     const QRectF& rect,
                     const QPointF& target,
                     const DialogueBubbleRenderer::Style& style,
                     bool isLeft)
    {
        painter.save();
        painter.setBrush(fillColorForBubbleType(QStringLiteral("thought"), style));
        painter.setPen(QPen(style.strokeColor, style.strokeWidth));

        const QPointF base = bubbleTailBasePoint(rect, isLeft, 0.2);
        const QPointF mid1 = base + (target - base) * 0.5;
        painter.drawEllipse(mid1, 5, 4);
        painter.drawEllipse(target + (base - target) * 0.12, 3, 2.5);

        painter.restore();
    }

    bool rectsIntersect(const QRectF& a, const QRectF& b)
    {
        return a.intersects(b);
    }

    bool isEllipsisOnlyText(const QString& text)
    {
        const QString trimmed = text.trimmed();
        if (trimmed.isEmpty()) {
            return true;
        }

        bool hasContent = false;
        for (const QChar& ch : trimmed) {
            if (ch.isSpace()) {
                continue;
            }
            if (ch == QLatin1Char('.') ||
                ch == QLatin1Char('·') ||
                ch == QChar(0x2026) ||
                ch == QChar(0x3002) ||
                ch == QChar(0xFF0E)) {
                continue;
            }
            hasContent = true;
            break;
        }
        return !hasContent;
    }

    bool shouldSkipBubbleRendering(const BubbleLayout& layout)
    {
        const QString speaker = layout.speaker.trimmed().toLower();
        if (speaker == QStringLiteral("narration")
            || speaker == QStringLiteral("旁白")
            || speaker == QStringLiteral("叙述")) {
            return true;
        }

        if (layout.bubbleType == QStringLiteral("narration")) {
            return true;
        }

        return isEllipsisOnlyText(layout.text);
    }
}

DialogueBubbleRenderer::Style& DialogueBubbleRenderer::mutableStyle()
{
    static Style s;
    return s;
}

QList<BubbleLayout> DialogueBubbleRenderer::layoutBubblesInternal(const LayoutContext& context)
{
    QList<BubbleLayout> layouts;
    if (!context.dialogues || context.imageWidth <= 0 || context.imageHeight <= 0) {
        return layouts;
    }

    const Style& style = mutableStyle();
    int maxBubbleWidth = static_cast<int>(context.imageWidth * style.maxBubbleWidthRatio);
    maxBubbleWidth = qBound(100, maxBubbleWidth, 500);

    qreal leftColumnY = style.topMargin;
    qreal rightColumnY = style.topMargin;
    layouts.reserve(context.dialogues->size());

    for (int i = 0; i < context.dialogues->size(); ++i) {
        if (context.dialogues->at(i).text.trimmed().isEmpty()) {
            continue;
        }
        LOG_DEBUG("DialogueBubbleRenderer", QString("layout input: index=%1, speaker=%2, bubbleType=%3, text=%4, speakerSide=%5")
            .arg(i)
            .arg(context.dialogues->at(i).speaker.isEmpty() ? QStringLiteral("(empty)") : context.dialogues->at(i).speaker)
            .arg(context.dialogues->at(i).bubbleType.isEmpty() ? QStringLiteral("(empty)") : context.dialogues->at(i).bubbleType)
            .arg(context.dialogues->at(i).text.left(80))
            .arg(context.dialogues->at(i).speakerSide.isEmpty() ? QStringLiteral("(empty)") : context.dialogues->at(i).speakerSide));
        layouts.append(createBubbleLayout(
            context.dialogues->at(i),
            i,
            maxBubbleWidth,
            leftColumnY,
            rightColumnY,
            context.imageWidth,
            context.imageHeight,
            context));
    }

    return layouts;
}

BubbleLayout DialogueBubbleRenderer::createBubbleLayout(const DialogueLine& dialogue,
                                                        int dialogueIndex,
                                                        int maxBubbleWidth,
                                                        qreal& leftColumnY,
                                                        qreal& rightColumnY,
                                                        int imageWidth,
                                                        int imageHeight,
                                                        const LayoutContext& context)
{
    const bool hasSpeakerSide = !dialogue.speakerSide.trimmed().isEmpty();

    if (dialogue.bubbleType == QStringLiteral("narration")) {
        if (context.usePanelPositioning && context.characters && hasSpeakerSide) {
            const QString shotType = context.shotType ? *context.shotType : QString();
            const QString cameraAngle = context.cameraAngle ? *context.cameraAngle : QString();
            return createPanelBubbleLayout(
                dialogue,
                dialogueIndex,
                maxBubbleWidth,
                imageWidth,
                imageHeight,
                *context.characters,
                shotType,
                cameraAngle);
        }
        return createNarrationBubbleLayout(
            dialogue,
            dialogueIndex,
            maxBubbleWidth,
            leftColumnY,
            rightColumnY,
            imageWidth,
            imageHeight);
    }

    if (context.usePanelPositioning && context.characters) {
        const QString shotType = context.shotType ? *context.shotType : QString();
        const QString cameraAngle = context.cameraAngle ? *context.cameraAngle : QString();
        return createPanelBubbleLayout(
            dialogue,
            dialogueIndex,
            maxBubbleWidth,
            imageWidth,
            imageHeight,
            *context.characters,
            shotType,
            cameraAngle);
    }

    return createDefaultBubbleLayout(
        dialogue,
        dialogueIndex,
        maxBubbleWidth,
        leftColumnY,
        rightColumnY,
        imageWidth,
        imageHeight);
}

BubbleLayout DialogueBubbleRenderer::createNarrationBubbleLayout(const DialogueLine& dialogue,
                                                                 int dialogueIndex,
                                                                 int maxBubbleWidth,
                                                                 qreal& leftColumnY,
                                                                 qreal& rightColumnY,
                                                                 int imageWidth,
                                                                 int imageHeight)
{
    const Style& style = mutableStyle();
    const QRectF textRect = calculateBubbleRect(dialogue.text, maxBubbleWidth);
    BubbleLayout layout = makeBaseBubbleLayout(dialogue, style);
    layout.tailTarget = QPointF(imageWidth * 0.50, imageHeight * 0.10);

    const qreal centerX = (imageWidth - textRect.width()) / 2.0;
    const qreal centerY = qMax<qreal>(style.topMargin, imageHeight * 0.05);
    layout.rect = QRectF(centerX, centerY, textRect.width(), textRect.height());

    advanceBubbleColumn(leftColumnY, layout.rect, style);
    advanceBubbleColumn(rightColumnY, layout.rect, style);

    LOG_DEBUG("DialogueBubbleRenderer", QString("createNarrationBubbleLayout: dialogueIndex=%1, rect=%2, tail=%3")
        .arg(dialogueIndex)
        .arg(rectToString(layout.rect))
        .arg(pointToString(layout.tailTarget)));

    return layout;
}

BubbleLayout DialogueBubbleRenderer::createDefaultBubbleLayout(const DialogueLine& dialogue,
                                                                int dialogueIndex,
                                                                int maxBubbleWidth,
                                                                qreal& leftColumnY,
                                                                qreal& rightColumnY,
                                                                int imageWidth,
                                                                int imageHeight)
{
    const Style& style = mutableStyle();
    const QRectF textRect = calculateBubbleRect(dialogue.text, maxBubbleWidth);
    BubbleLayout layout = makeBaseBubbleLayout(dialogue, style);
    layout.tailTarget = QPointF((dialogueIndex % 2 == 0) ? imageWidth * 0.25 : imageWidth * 0.75, imageHeight * 0.55);

    const bool placeLeft = (dialogueIndex % 2 == 0);
    qreal x = placeLeft ? style.sideMargin : imageWidth - textRect.width() - style.sideMargin;
    qreal y = placeLeft ? leftColumnY : rightColumnY;
    if (placeLeft) {
        advanceBubbleColumn(leftColumnY, QRectF(x, y, textRect.width(), textRect.height()), style);
    } else {
        advanceBubbleColumn(rightColumnY, QRectF(x, y, textRect.width(), textRect.height()), style);
    }

    layout.rect = QRectF(x, y, textRect.width(), textRect.height());
    return layout;
}

BubbleLayout DialogueBubbleRenderer::createPanelBubbleLayout(const DialogueLine& dialogue,
                                                              int dialogueIndex,
                                                              int maxBubbleWidth,
                                                              int imageWidth,
                                                              int imageHeight,
                                                              const QList<CharacterPose>& characters,
                                                              const QString& shotType,
                                                              const QString& cameraAngle)
{
    const Style& style = mutableStyle();
    const QRectF textRect = calculateBubbleRect(dialogue.text, maxBubbleWidth);
    BubbleLayout layout = makeBaseBubbleLayout(dialogue, style);

    const CharacterPosition charPos = characterPositionFromSpeakerSide(dialogue.speakerSide.isEmpty()
        ? QStringLiteral("auto")
        : dialogue.speakerSide);
    const CharacterPosition resolvedPos = (charPos == CharacterPosition::Unknown)
        ? inferCharacterPosition(dialogue.speaker, dialogueIndex, characters, shotType, cameraAngle)
        : charPos;
    QPointF facePosition = calculateFacePosition(resolvedPos, imageWidth, imageHeight, shotType);
    if (facePosition.isNull()) {
        facePosition = QPointF(imageWidth * 0.50, imageHeight * 0.55);
    }
    LOG_DEBUG("DialogueBubbleRenderer", QString("createPanelBubbleLayout: speaker=%1, dialogueIndex=%2, charPos=%3, face=%4, shotType=%5, cameraAngle=%6")
        .arg(dialogue.speaker.isEmpty() ? QStringLiteral("(empty)") : dialogue.speaker)
        .arg(dialogueIndex)
        .arg(characterPositionToString(resolvedPos))
        .arg(pointToString(facePosition))
        .arg(shotType.isEmpty() ? QStringLiteral("(empty)") : shotType)
        .arg(cameraAngle.isEmpty() ? QStringLiteral("(empty)") : cameraAngle));
    layout.tailTarget = facePosition;
    layout.rect = buildPanelBubbleRect(textRect, resolvedPos, facePosition, imageWidth, imageHeight);
    return layout;
}

CharacterPosition DialogueBubbleRenderer::characterPositionFromSpeakerSide(const QString& speakerSide)
{
    const QString normalized = DialogueSpeakerSideUtils::normalize(speakerSide);
    if (normalized == QStringLiteral("left")) {
        return CharacterPosition::Left;
    }
    if (normalized == QStringLiteral("right")) {
        return CharacterPosition::Right;
    }
    if (normalized == QStringLiteral("center")) {
        return CharacterPosition::Center;
    }
    return CharacterPosition::Unknown;
}

QRectF DialogueBubbleRenderer::buildPanelBubbleRect(const QRectF& textRect,
                                                    CharacterPosition charPos,
                                                    const QPointF& facePosition,
                                                    int imageWidth,
                                                    int imageHeight)
{
    const Style& style = mutableStyle();
    const qreal bubbleGap = qBound<qreal>(18.0, imageHeight * 0.04, 48.0);
    const qreal maxFaceDistance = qBound<qreal>(110.0, imageHeight * 0.22, 190.0);
    const QRectF safeRect = estimateSpeakerSafeRect(charPos, facePosition, textRect);
    const QRectF alignedRect = alignBubbleRectToSpeaker(textRect, charPos, facePosition);
    QRectF bubbleRect = alignedRect;

    qreal y = qMax<qreal>(style.topMargin, facePosition.y() - textRect.height() - bubbleGap);
    bubbleRect.moveTop(y);
    const QRectF pulledRect = pullRectTowardTarget(bubbleRect, facePosition, maxFaceDistance, imageWidth, imageHeight);
    const QRectF finalRect = avoidSpeakerOverlap(pulledRect, safeRect, charPos, imageWidth, imageHeight);

    LOG_DEBUG("DialogueBubbleRenderer", QString("buildPanelBubbleRect: charPos=%1, face=%2, textRect=%3, aligned=%4, pulled=%5, safe=%6, final=%7")
        .arg(characterPositionToString(charPos))
        .arg(pointToString(facePosition))
        .arg(rectToString(textRect))
        .arg(rectToString(alignedRect))
        .arg(rectToString(pulledRect))
        .arg(rectToString(safeRect))
        .arg(rectToString(finalRect)));

    return finalRect;
}

QRectF DialogueBubbleRenderer::estimateSpeakerSafeRect(CharacterPosition charPos,
                                                      const QPointF& facePosition,
                                                      const QRectF& bubbleRect)
{
    const qreal faceWidth = qBound<qreal>(72.0, bubbleRect.width() * 0.72, 132.0);
    const qreal faceHeight = qBound<qreal>(72.0, bubbleRect.height() * 0.66, 120.0);
    const qreal upperBodyWidth = qBound<qreal>(90.0, bubbleRect.width() * 0.95, 180.0);
    const qreal upperBodyHeight = qBound<qreal>(110.0, bubbleRect.height() * 0.95, 180.0);

    qreal faceLeft = facePosition.x() - faceWidth / 2.0;
    if (charPos == CharacterPosition::Left) {
        faceLeft = facePosition.x() - faceWidth * 0.35;
    } else if (charPos == CharacterPosition::Right) {
        faceLeft = facePosition.x() - faceWidth * 0.65;
    }

    const QRectF faceRect(faceLeft, qMax<qreal>(0.0, facePosition.y() - faceHeight * 0.72), faceWidth, faceHeight);
    const QRectF upperBodyRect(facePosition.x() - upperBodyWidth / 2.0,
                               faceRect.bottom() - faceHeight * 0.10,
                               upperBodyWidth,
                               upperBodyHeight);
    return faceRect.united(upperBodyRect);
}

QRectF DialogueBubbleRenderer::avoidSpeakerOverlap(const QRectF& bubbleRect,
                                                   const QRectF& safeRect,
                                                   CharacterPosition charPos,
                                                   int imageWidth,
                                                   int imageHeight)
{
    if (!rectsIntersect(bubbleRect, safeRect)) {
        return bubbleRect;
    }

    const qreal liftStep = qMax<qreal>(12.0, imageHeight * 0.03);
    QRectF lifted = bubbleRect;
    for (int i = 0; i < 4; ++i) {
        lifted.translate(0, -liftStep);
        lifted = clampBubbleRectToImage(lifted, imageWidth, imageHeight);
        if (!rectsIntersect(lifted, safeRect)) {
            return lifted;
        }
    }

    const qreal sideStep = qMax<qreal>(14.0, imageWidth * 0.03);
    QRectF sideShifted = bubbleRect;
    for (int i = 0; i < 3; ++i) {
        sideShifted.translate((charPos == CharacterPosition::Right ? -sideStep : sideStep), -liftStep * 0.5);
        sideShifted = clampBubbleRectToImage(sideShifted, imageWidth, imageHeight);
        if (!rectsIntersect(sideShifted, safeRect)) {
            return sideShifted;
        }
    }

    return bubbleRect;
}

QRectF DialogueBubbleRenderer::pullRectTowardTarget(const QRectF& rect,
                                                    const QPointF& target,
                                                    qreal maxDistance,
                                                    int imageWidth,
                                                    int imageHeight)
{
    QRectF bubbleRect = clampBubbleRectToImage(rect, imageWidth, imageHeight);
    const qreal currentDistance = rectCenterDistance(bubbleRect, target);
    if (currentDistance <= maxDistance) {
        return bubbleRect;
    }

    const QPointF bubbleCenter = bubbleRect.center();
    const QPointF direction = bubbleCenter - target;
    const qreal length = qSqrt(direction.x() * direction.x() + direction.y() * direction.y());
    if (length <= 0.001) {
        return bubbleRect;
    }

    const qreal pull = qMin<qreal>(currentDistance - maxDistance, currentDistance - 1.0);
    const qreal scale = pull / length;
    bubbleRect.translate(-direction.x() * scale, -direction.y() * scale);
    return clampBubbleRectToImage(bubbleRect, imageWidth, imageHeight);
}

void DialogueBubbleRenderer::drawOutlinedBubble(QPainter& painter,
                                                 const QColor& fillColor,
                                                 const Style& style,
                                                 std::function<void(QPainter&)> shapeDrawer)
{
    painter.save();
    painter.setBrush(fillColor);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth));
    shapeDrawer(painter);
    painter.restore();
}

QImage DialogueBubbleRenderer::render(const QImage& baseImage, const QList<DialogueLine>& dialogues, const QSize& imageSize)
{
    if (dialogues.isEmpty()) {
        return baseImage;
    }

    QImage canvas = imageSize.isValid()
        ? baseImage.scaled(imageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        : baseImage;

    if (canvas.isNull()) {
        LOG_WARNING("DialogueBubbleRenderer", "Base image is null, cannot render bubbles");
        return baseImage;
    }

    LayoutContext context;
    context.dialogues = &dialogues;
    context.imageWidth = canvas.width();
    context.imageHeight = canvas.height();
    QList<BubbleLayout> layouts = layoutBubblesInternal(context);
    if (layouts.isEmpty()) {
        return canvas;
    }

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const Style& style = mutableStyle();
    for (const BubbleLayout& layout : layouts) {
        drawBubble(painter, layout, style);
    }

    painter.end();
    return canvas;
}

QImage DialogueBubbleRenderer::renderForPanel(const Panel& panel, const QImage& baseImage)
{
    if (panel.dialogue().isEmpty()) {
        return baseImage;
    }

    if (baseImage.isNull()) {
        LOG_WARNING("DialogueBubbleRenderer", "Base image is null, cannot render bubbles");
        return baseImage;
    }

    QImage canvas = baseImage;

    const QList<DialogueLine> dialogues = panel.dialogue();
    const QList<CharacterPose> characters = panel.characters();
    const QString shotType = panel.shotType();
    const QString cameraAngle = panel.cameraAngle();

    LayoutContext context;
    context.dialogues = &dialogues;
    context.characters = &characters;
    context.shotType = &shotType;
    context.cameraAngle = &cameraAngle;
    context.imageWidth = canvas.width();
    context.imageHeight = canvas.height();
    context.usePanelPositioning = true;

    QList<BubbleLayout> layouts = layoutBubblesInternal(context);

    if (layouts.isEmpty()) {
        return canvas;
    }

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const Style& style = mutableStyle();
    for (const BubbleLayout& layout : layouts) {
        drawBubble(painter, layout, style);
    }

    painter.end();
    return canvas;
}

CharacterPosition DialogueBubbleRenderer::inferCharacterPosition(
    const QString& speaker,
    int dialogueIndex,
    const QList<CharacterPose>& characters,
    const QString& shotType,
    const QString& cameraAngle)
{
    Q_UNUSED(cameraAngle);

    QString type = shotType.toLower().trimmed();

    if (type.contains(QStringLiteral("close-up")) ||
        type.contains(QStringLiteral("extreme")) ||
        type.contains(QStringLiteral("特写"))) {
        return CharacterPosition::Center;
    }

    if (characters.size() >= 2) {
        const QString normalizedSpeaker = normalizeSpeakerKey(speaker);
        for (int c = 0; c < characters.size(); ++c) {
            const QString normalizedName = normalizeSpeakerKey(characters[c].name);
            const QString normalizedId = normalizeSpeakerKey(characters[c].charId);
            if (!normalizedSpeaker.isEmpty() &&
                (normalizedSpeaker == normalizedName
                 || normalizedSpeaker == normalizedId
                 || normalizedName.startsWith(normalizedSpeaker)
                 || normalizedSpeaker.startsWith(normalizedName)
                 || normalizedId.startsWith(normalizedSpeaker)
                 || normalizedSpeaker.startsWith(normalizedId))) {
                LOG_DEBUG("DialogueBubbleRenderer", QString("inferCharacterPosition matched: speaker=%1 normalizedSpeaker=%2 matchedName=%3 matchedId=%4 result=%5")
                    .arg(speaker.isEmpty() ? QStringLiteral("(empty)") : speaker)
                    .arg(normalizedSpeaker.isEmpty() ? QStringLiteral("(empty)") : normalizedSpeaker)
                    .arg(characters[c].name.isEmpty() ? QStringLiteral("(empty)") : characters[c].name)
                    .arg(characters[c].charId.isEmpty() ? QStringLiteral("(empty)") : characters[c].charId)
                    .arg(characterPositionToString((c % 2 == 0) ? CharacterPosition::Left : CharacterPosition::Right)));
                return (c % 2 == 0) ? CharacterPosition::Left : CharacterPosition::Right;
            }
        }
    }

    LOG_DEBUG("DialogueBubbleRenderer", QString("inferCharacterPosition fallback: speaker=%1, dialogueIndex=%2, characters=%3, result=%4")
        .arg(speaker.isEmpty() ? QStringLiteral("(empty)") : speaker)
        .arg(dialogueIndex)
        .arg(characters.size())
        .arg(characterPositionToString((dialogueIndex % 2 == 0) ? CharacterPosition::Left : CharacterPosition::Right)));
    return (dialogueIndex % 2 == 0) ? CharacterPosition::Left : CharacterPosition::Right;
}

QList<BubbleLayout> DialogueBubbleRenderer::layoutBubbles(const QList<DialogueLine>& dialogues, int imageWidth, int imageHeight)
{
    LayoutContext context;
    context.dialogues = &dialogues;
    context.imageWidth = imageWidth;
    context.imageHeight = imageHeight;
    return layoutBubblesInternal(context);
}

QPointF DialogueBubbleRenderer::calculateFacePosition(
    CharacterPosition charPos,
    int imageWidth,
    int imageHeight,
    const QString& shotType)
{
    Q_UNUSED(shotType);

    const double characterY = imageHeight * 0.30;

    double characterX;
    switch (charPos) {
        case CharacterPosition::Left:
            characterX = imageWidth * 0.28;
            break;
        case CharacterPosition::Right:
            characterX = imageWidth * 0.72;
            break;
        case CharacterPosition::Center:
        default:
            characterX = imageWidth * 0.50;
            break;
    }

    return QPointF(characterX, characterY);
}

QRectF DialogueBubbleRenderer::calculateBubbleRect(const QString& text, int maxBubbleWidth)
{
    const Style& style = mutableStyle();

    QFont textFont;
    textFont.setPointSize(style.fontSize);
    QFontMetrics fm(textFont);

    const int contentWidth = qMax(60, maxBubbleWidth - style.padding * 2);
    const QStringList lines = wrapText(text, fm, contentWidth);
    const int textHeight = lines.isEmpty()
        ? fm.height()
        : lines.size() * fm.height() + (lines.size() - 1) * style.lineSpacing;
    const int totalHeight = style.padding * 2 + textHeight;
    int totalWidth = 0;
    for (const QString& line : lines) {
        totalWidth = qMax(totalWidth, fm.horizontalAdvance(line));
    }
    totalWidth = qBound(80, totalWidth + style.padding * 2, maxBubbleWidth);

    return QRectF(0, 0, totalWidth, totalHeight);
}

void DialogueBubbleRenderer::drawBubble(QPainter& painter, const BubbleLayout& layout, const Style& style)
{
    if (shouldSkipBubbleRendering(layout)) {
        LOG_DEBUG("DialogueBubbleRenderer", QString("drawBubble skipped: speaker=%1, bubbleType=%2, text=%3")
            .arg(layout.speaker.isEmpty() ? QStringLiteral("(empty)") : layout.speaker)
            .arg(layout.bubbleType.isEmpty() ? QStringLiteral("(empty)") : layout.bubbleType)
            .arg(layout.text.isEmpty() ? QStringLiteral("(empty)") : layout.text.left(80)));
        return;
    }

    if (layout.bubbleType == QStringLiteral("thought")) {
        drawThoughtBubble(painter, layout.rect, layout.tailTarget, style);
        return;
    }

    if (layout.bubbleType == QStringLiteral("scream") || layout.bubbleType == QStringLiteral("shout")) {
        drawScreamBubble(painter, layout.rect, layout.tailTarget, style);
        return;
    }

    drawSpeechBubble(painter, layout.rect, layout.tailTarget, style);

    painter.setPen(style.textColor);
    QFont textFont;
    textFont.setPointSize(layout.fontSize);
    painter.setFont(textFont);
    QFontMetrics fm(textFont);

    qreal textY = layout.rect.top() + style.padding;

    QStringList lines = wrapText(layout.text, fm, static_cast<int>(layout.rect.width() - style.padding * 2));
    for (const QString& line : lines) {
        QRectF lineRect(layout.rect.left() + style.padding, textY,
                        layout.rect.width() - style.padding * 2, fm.height());
        painter.drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, line);
        textY += fm.height() + style.lineSpacing;
    }
}

void DialogueBubbleRenderer::drawSpeechBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style)
{
    const QColor fill = fillColorForType(QStringLiteral("speech"), style);
    drawOutlinedBubble(painter, fill, style, [&](QPainter& p) {
        QPainterPath path;
        path.addRoundedRect(rect, style.cornerRadius, style.cornerRadius);
        p.drawPath(path);
    });

    const bool isLeft = tailTarget.x() < rect.center().x();
    drawTail(painter, rect, tailTarget, style, isLeft);
}

void DialogueBubbleRenderer::drawThoughtBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style)
{
    const QColor fill = fillColorForType(QStringLiteral("thought"), style);
    drawOutlinedBubble(painter, fill, style, [&](QPainter& p) {
        QPainterPath path;
        path.addRoundedRect(rect, style.cornerRadius * 1.5, style.cornerRadius * 1.5);
        p.drawPath(path);
    });

    const bool isLeft = tailTarget.x() < rect.center().x();
    drawThoughtTail(painter, rect, tailTarget, style, isLeft);
}

void DialogueBubbleRenderer::drawScreamBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style)
{
    const QColor fill = fillColorForType(QStringLiteral("scream"), style);
    drawOutlinedBubble(painter, fill, style, [&](QPainter& p) {
        p.setPen(QPen(style.strokeColor, style.strokeWidth * 1.5));

        const double cx = rect.center().x();
        const double cy = rect.center().y();
        const double rx = rect.width() / 2.0;
        const double ry = rect.height() / 2.0;

        QPainterPath path;
        const int spikes = 16;
        const double innerRatio = 0.85;
        for (int i = 0; i < spikes * 2; ++i) {
            const double angle = M_PI * i / spikes;
            const double r = (i % 2 == 0) ? 1.0 : innerRatio;
            const double x = cx + rx * r * qCos(angle);
            const double y = cy + ry * r * qSin(angle);
            if (i == 0) {
                path.moveTo(x, y);
            } else {
                path.lineTo(x, y);
            }
        }
        path.closeSubpath();
        p.drawPath(path);
    });

    const bool isLeft = tailTarget.x() < rect.center().x();
    drawTail(painter, rect, tailTarget, style, isLeft);
}

void DialogueBubbleRenderer::drawTail(QPainter& painter, const QRectF& rect, const QPointF& target, const Style& style, bool isLeft)
{
    drawTriangleTail(painter, rect, target, style, isLeft);
}

void DialogueBubbleRenderer::drawThoughtTail(QPainter& painter, const QRectF& rect, const QPointF& target, const Style& style, bool isLeft)
{
    drawDotTail(painter, rect, target, style, isLeft);
}

QStringList DialogueBubbleRenderer::wrapText(const QString& text, const QFontMetrics& fm, int maxWidth)
{
    if (text.isEmpty()) {
        return QStringList();
    }

    maxWidth = qMax(maxWidth, 30);

    QStringList lines;
    QString currentLine;

    for (const QChar& ch : text) {
        if (ch == QLatin1Char('\n')) {
            lines.append(currentLine);
            currentLine.clear();
            continue;
        }

        QString testLine = currentLine + ch;
        if (fm.horizontalAdvance(testLine) > maxWidth && !currentLine.isEmpty()) {
            lines.append(currentLine);
            currentLine = ch;
        } else {
            currentLine = testLine;
        }
    }

    if (!currentLine.isEmpty()) {
        lines.append(currentLine);
    }

    return lines;
}

QColor DialogueBubbleRenderer::fillColorForType(const QString& bubbleType, const Style& style)
{
    return fillColorForBubbleType(bubbleType, style);
}
