#include "utils/DialogueBubbleRenderer.h"
#include "utils/Logger.h"
#include <algorithm>
#include <QtMath>

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

    qreal startY = style.topMargin;
    if (context.usePanelPositioning) {
        startY = qMax<qreal>(style.topMargin, context.imageHeight * 0.14);
    }

    qreal leftColumnY = startY;
    qreal rightColumnY = startY;
    layouts.reserve(context.dialogues->size());

    for (int i = 0; i < context.dialogues->size(); ++i) {
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
    const Style& style = mutableStyle();

    BubbleLayout layout;
    layout.text = dialogue.text;
    layout.speaker = dialogue.speaker;
    layout.bubbleType = dialogue.bubbleType;
    layout.fontSize = style.fontSize;

    CharacterPosition charPos = CharacterPosition::Unknown;
    if (context.usePanelPositioning && context.characters) {
        const QString shotType = context.shotType ? *context.shotType : QString();
        const QString cameraAngle = context.cameraAngle ? *context.cameraAngle : QString();
        charPos = inferCharacterPosition(dialogue.speaker, dialogueIndex, *context.characters, shotType, cameraAngle);
        layout.tailTarget = calculateFacePosition(charPos, imageWidth, imageHeight, shotType);
    }

    QRectF textRect = calculateBubbleRect(dialogue.text, maxBubbleWidth);
    const bool isLeft = context.usePanelPositioning
        ? (charPos == CharacterPosition::Left || (charPos == CharacterPosition::Unknown && dialogueIndex % 2 == 0))
        : (dialogueIndex % 2 == 0);
    const bool isCenter = context.usePanelPositioning && charPos == CharacterPosition::Center;

    qreal x = 0;
    qreal y = 0;
    if (isCenter) {
        x = (imageWidth - textRect.width()) / 2.0;
        y = leftColumnY;
        leftColumnY = y + textRect.height() + style.bubbleSpacing;
    } else if (isLeft) {
        x = style.sideMargin;
        y = leftColumnY;
        leftColumnY = y + textRect.height() + style.bubbleSpacing;
    } else {
        x = imageWidth - textRect.width() - style.sideMargin;
        y = rightColumnY;
        rightColumnY = y + textRect.height() + style.bubbleSpacing;
    }

    layout.rect = QRectF(x, y, textRect.width(), textRect.height());

    if (!context.usePanelPositioning) {
        const double characterX = isLeft ? imageWidth * 0.25 : imageWidth * 0.75;
        const double characterY = imageHeight * 0.55;
        layout.tailTarget = QPointF(characterX, characterY);
    } else if (layout.tailTarget.isNull()) {
        layout.tailTarget = QPointF(imageWidth * 0.50, imageHeight * 0.55);
    }

    return layout;
}

void DialogueBubbleRenderer::drawBubbleFrame(QPainter& painter, const BubbleLayout& layout, const Style& style)
{
    if (layout.bubbleType == QStringLiteral("thought")) {
        drawThoughtBubble(painter, layout.rect, layout.tailTarget, style);
    } else if (layout.bubbleType == QStringLiteral("narration")) {
        drawNarrationBox(painter, layout.rect, style);
    } else if (layout.bubbleType == QStringLiteral("scream") || layout.bubbleType == QStringLiteral("shout")) {
        drawScreamBubble(painter, layout.rect, layout.tailTarget, style);
    } else {
        drawSpeechBubble(painter, layout.rect, layout.tailTarget, style);
    }
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

    QImage canvas = baseImage;

    if (canvas.isNull()) {
        LOG_WARNING("DialogueBubbleRenderer", "Base image is null, cannot render bubbles");
        return baseImage;
    }

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

QList<BubbleLayout> DialogueBubbleRenderer::layoutBubblesForPanel(
    const QList<DialogueLine>& dialogues,
    const QList<CharacterPose>& characters,
    const QString& shotType,
    const QString& cameraAngle,
    int imageWidth,
    int imageHeight)
{
    LayoutContext context;
    context.dialogues = &dialogues;
    context.characters = &characters;
    context.shotType = &shotType;
    context.cameraAngle = &cameraAngle;
    context.imageWidth = imageWidth;
    context.imageHeight = imageHeight;
    context.usePanelPositioning = true;
    return layoutBubblesInternal(context);
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
        for (int c = 0; c < characters.size(); ++c) {
            if (!speaker.isEmpty() &&
                (characters[c].name.contains(speaker) || speaker.contains(characters[c].name))) {
                return (c % 2 == 0) ? CharacterPosition::Left : CharacterPosition::Right;
            }
        }
    }

    return (dialogueIndex % 2 == 0) ? CharacterPosition::Left : CharacterPosition::Right;
}

QPointF DialogueBubbleRenderer::calculateFacePosition(
    CharacterPosition charPos,
    int imageWidth,
    int imageHeight,
    const QString& shotType)
{
    Q_UNUSED(shotType);

    double characterY = imageHeight * 0.32;

    double characterX;
    switch (charPos) {
        case CharacterPosition::Left:
            characterX = imageWidth * 0.24;
            break;
        case CharacterPosition::Right:
            characterX = imageWidth * 0.76;
            break;
        case CharacterPosition::Center:
        default:
            characterX = imageWidth * 0.50;
            break;
    }

    return QPointF(characterX, characterY);
}

QList<BubbleLayout> DialogueBubbleRenderer::layoutBubbles(const QList<DialogueLine>& dialogues, int imageWidth, int imageHeight)
{
    LayoutContext context;
    context.dialogues = &dialogues;
    context.imageWidth = imageWidth;
    context.imageHeight = imageHeight;
    return layoutBubblesInternal(context);
}

QRectF DialogueBubbleRenderer::calculateBubbleRect(const QString& text, int maxBubbleWidth)
{
    const Style& style = mutableStyle();

    QFont textFont;
    textFont.setPointSize(style.fontSize);
    QFontMetrics fm(textFont);

    int contentWidth = maxBubbleWidth - style.padding * 2;
    contentWidth = qMax(contentWidth, 60);

    QStringList lines = wrapText(text, fm, contentWidth);

    int textHeight = 0;
    if (!lines.isEmpty()) {
        textHeight = lines.size() * fm.height() + (lines.size() - 1) * style.lineSpacing;
    }
    textHeight = qMax(textHeight, fm.height());

    int totalHeight = style.padding + textHeight + style.padding;

    int maxLineWidth = 0;
    for (const QString& line : lines) {
        maxLineWidth = qMax(maxLineWidth, fm.horizontalAdvance(line));
    }

    int totalWidth = maxLineWidth + style.padding * 2;
    totalWidth = qBound(80, totalWidth, maxBubbleWidth);

    return QRectF(0, 0, totalWidth, totalHeight);
}

void DialogueBubbleRenderer::drawBubble(QPainter& painter, const BubbleLayout& layout, const Style& style)
{
    drawBubbleFrame(painter, layout, style);

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
    painter.save();

    QColor fill = fillColorForType(QStringLiteral("speech"), style);
    painter.setBrush(fill);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth));

    QPainterPath path;
    path.addRoundedRect(rect, style.cornerRadius, style.cornerRadius);
    painter.drawPath(path);

    bool isLeft = tailTarget.x() < rect.center().x();
    drawTail(painter, rect, tailTarget, style, isLeft);

    painter.restore();
}

void DialogueBubbleRenderer::drawThoughtBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style)
{
    painter.save();

    QColor fill = fillColorForType(QStringLiteral("thought"), style);
    painter.setBrush(fill);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth));

    QPainterPath path;
    path.addRoundedRect(rect, style.cornerRadius * 1.5, style.cornerRadius * 1.5);
    painter.drawPath(path);

    bool isLeft = tailTarget.x() < rect.center().x();
    drawThoughtTail(painter, rect, tailTarget, style, isLeft);

    painter.restore();
}

void DialogueBubbleRenderer::drawNarrationBox(QPainter& painter, const QRectF& rect, const Style& style)
{
    painter.save();

    QColor fill = fillColorForType(QStringLiteral("narration"), style);
    painter.setBrush(fill);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth));

    QPainterPath path;
    path.addRect(rect);
    painter.drawPath(path);

    painter.restore();
}

void DialogueBubbleRenderer::drawScreamBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style)
{
    painter.save();

    QColor fill = fillColorForType(QStringLiteral("scream"), style);
    painter.setBrush(fill);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth * 1.5));

    double cx = rect.center().x();
    double cy = rect.center().y();
    double rx = rect.width() / 2.0;
    double ry = rect.height() / 2.0;

    QPainterPath path;
    const int spikes = 16;
    const double innerRatio = 0.85;
    for (int i = 0; i < spikes * 2; ++i) {
        double angle = M_PI * i / spikes;
        double r = (i % 2 == 0) ? 1.0 : innerRatio;
        double x = cx + rx * r * qCos(angle);
        double y = cy + ry * r * qSin(angle);
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    path.closeSubpath();
    painter.drawPath(path);

    bool isLeft = tailTarget.x() < rect.center().x();
    drawTail(painter, rect, tailTarget, style, isLeft);

    painter.restore();
}

void DialogueBubbleRenderer::drawTail(QPainter& painter, const QRectF& rect, const QPointF& target, const Style& style, bool isLeft)
{
    painter.save();

    QColor fill = fillColorForType(QStringLiteral("speech"), style);
    painter.setBrush(fill);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth));

    qreal tailBaseY = rect.bottom() - rect.height() * 0.3;
    QPointF baseCenter = isLeft
        ? QPointF(rect.left(), tailBaseY)
        : QPointF(rect.right(), tailBaseY);

    QPointF baseTop = baseCenter + QPointF(0, -6);
    QPointF baseBottom = baseCenter + QPointF(0, 6);

    QPainterPath tailPath;
    tailPath.moveTo(baseTop);
    tailPath.lineTo(target);
    tailPath.lineTo(baseBottom);
    tailPath.closeSubpath();
    painter.drawPath(tailPath);

    painter.restore();
}

void DialogueBubbleRenderer::drawThoughtTail(QPainter& painter, const QRectF& rect, const QPointF& target, const Style& style, bool isLeft)
{
    painter.save();

    QColor fill = fillColorForType(QStringLiteral("thought"), style);
    painter.setBrush(fill);
    painter.setPen(QPen(style.strokeColor, style.strokeWidth));

    qreal tailBaseY = rect.bottom() - rect.height() * 0.2;
    QPointF base = isLeft
        ? QPointF(rect.left(), tailBaseY)
        : QPointF(rect.right(), tailBaseY);

    QPointF mid1 = base + (target - base) * 0.5;
    painter.drawEllipse(mid1, 5, 4);
    painter.drawEllipse(target + (base - target) * 0.12, 3, 2.5);

    painter.restore();
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
