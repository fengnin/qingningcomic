#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QColor>
#include <QRectF>
#include <QList>
#include <functional>
#include "models/Panel.h"

struct BubbleLayout {
    QRectF rect;
    QString text;
    QString speaker;
    QString bubbleType;
    QPointF tailTarget;
    int fontSize;
};

enum class CharacterPosition {
    Left,
    Right,
    Center,
    Unknown
};

class DialogueBubbleRenderer
{
public:
    static QImage render(const QImage& baseImage, const QList<DialogueLine>& dialogues, const QSize& imageSize = QSize());

    static QImage renderForPanel(const Panel& panel, const QImage& baseImage);

    struct Style {
        QColor fillColor = QColor(255, 255, 255, 230);
        QColor strokeColor = QColor(40, 40, 40, 200);
        QColor textColor = QColor(20, 20, 20);
        QColor thoughtFillColor = QColor(240, 245, 255, 230);
        QColor narrationFillColor = QColor(245, 245, 220, 230);
        QColor screamFillColor = QColor(255, 230, 230, 230);
        double strokeWidth = 2.0;
        double cornerRadius = 12.0;
        int fontSize = 16;
        int padding = 14;
        int lineSpacing = 4;
        int bubbleSpacing = 12;
        int topMargin = 20;
        int sideMargin = 20;
        double maxBubbleWidthRatio = 0.45;
    };

    static Style& mutableStyle();

private:
    struct LayoutContext {
        const QList<DialogueLine>* dialogues = nullptr;
        const QList<CharacterPose>* characters = nullptr;
        const QString* shotType = nullptr;
        const QString* cameraAngle = nullptr;
        int imageWidth = 0;
        int imageHeight = 0;
        bool usePanelPositioning = false;
    };

    static QList<BubbleLayout> layoutBubblesInternal(const LayoutContext& context);
    static BubbleLayout createBubbleLayout(const DialogueLine& dialogue,
                                           int dialogueIndex,
                                           int maxBubbleWidth,
                                           qreal& leftColumnY,
                                           qreal& rightColumnY,
                                           int imageWidth,
                                           int imageHeight,
                                           const LayoutContext& context);
    static BubbleLayout createDefaultBubbleLayout(const DialogueLine& dialogue,
                                                  int dialogueIndex,
                                                  int maxBubbleWidth,
                                                  qreal& leftColumnY,
                                                  qreal& rightColumnY,
                                                  int imageWidth,
                                                  int imageHeight);
    static BubbleLayout createPanelBubbleLayout(const DialogueLine& dialogue,
                                                int dialogueIndex,
                                                int maxBubbleWidth,
                                                int imageWidth,
                                                int imageHeight,
                                                const QList<CharacterPose>& characters,
                                                const QString& shotType,
                                                const QString& cameraAngle);
    static BubbleLayout createNarrationBubbleLayout(const DialogueLine& dialogue,
                                                    int dialogueIndex,
                                                    int maxBubbleWidth,
                                                    qreal& leftColumnY,
                                                    qreal& rightColumnY,
                                                    int imageWidth,
                                                    int imageHeight);
    static QRectF buildPanelBubbleRect(const QRectF& textRect,
                                       CharacterPosition charPos,
                                       const QPointF& facePosition,
                                       int imageWidth,
                                       int imageHeight);
    static QRectF estimateSpeakerSafeRect(CharacterPosition charPos,
                                          const QPointF& facePosition,
                                          const QRectF& bubbleRect);
    static QRectF avoidSpeakerOverlap(const QRectF& bubbleRect,
                                      const QRectF& safeRect,
                                      CharacterPosition charPos,
                                      int imageWidth,
                                      int imageHeight);
    static QRectF pullRectTowardTarget(const QRectF& rect,
                                       const QPointF& target,
                                       qreal maxDistance,
                                       int imageWidth,
                                       int imageHeight);
    static void drawOutlinedBubble(QPainter& painter,
                                   const QColor& fillColor,
                                   const Style& style,
                                   std::function<void(QPainter&)> shapeDrawer);

    static QList<BubbleLayout> layoutBubbles(const QList<DialogueLine>& dialogues, int imageWidth, int imageHeight);

    static CharacterPosition inferCharacterPosition(
        const QString& speaker,
        int dialogueIndex,
        const QList<CharacterPose>& characters,
        const QString& shotType,
        const QString& cameraAngle);

    static QPointF calculateFacePosition(
        CharacterPosition charPos,
        int imageWidth,
        int imageHeight,
        const QString& shotType);

    static CharacterPosition characterPositionFromSpeakerSide(const QString& speakerSide);

    static QRectF calculateBubbleRect(const QString& text, int maxBubbleWidth);
    static void drawBubble(QPainter& painter, const BubbleLayout& layout, const Style& style);
    static void drawSpeechBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style);
    static void drawThoughtBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style);
    static void drawScreamBubble(QPainter& painter, const QRectF& rect, const QPointF& tailTarget, const Style& style);
    static void drawTail(QPainter& painter, const QRectF& rect, const QPointF& target, const Style& style, bool isLeft);
    static void drawThoughtTail(QPainter& painter, const QRectF& rect, const QPointF& target, const Style& style, bool isLeft);
    static QStringList wrapText(const QString& text, const QFontMetrics& fm, int maxWidth);
    static QColor fillColorForType(const QString& bubbleType, const Style& style);
};
