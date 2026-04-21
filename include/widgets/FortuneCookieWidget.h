#ifndef FORTUNECOOKIEWIDGET_H
#define FORTUNECOOKIEWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QRandomGenerator>
#include <QVector>
#include <QPointF>

struct FortuneData {
    QString quote;
    QVector<int> luckyNumbers;
};

class FortuneCookieWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal cookieRotation READ cookieRotation WRITE setCookieRotation)
    Q_PROPERTY(qreal cookieScale READ cookieScale WRITE setCookieScale)
    Q_PROPERTY(qreal glowOpacity READ glowOpacity WRITE setGlowOpacity)

public:
    enum class CookieState {
        Unopened,
        Cracking,
        Opened
    };

    explicit FortuneCookieWidget(QWidget *parent = nullptr);
    ~FortuneCookieWidget();

    void crackCookie();
    void resetCookie();
    CookieState currentState() const { return m_state; }

    qreal cookieRotation() const { return m_rotation; }
    void setCookieRotation(qreal rotation);

    qreal cookieScale() const { return m_scale; }
    void setCookieScale(qreal scale);

    qreal glowOpacity() const { return m_glowOpacity; }
    void setGlowOpacity(qreal opacity);

signals:
    void fortuneRevealed(const FortuneData &fortune);
    void stateChanged(CookieState newState);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void initFortunes();
    void initAnimations();
    void updateAnimationTimer();
    FortuneData getRandomFortune();
    void updateCursor();
    
    void drawBackground(QPainter &painter);
    void drawGradientText(QPainter &painter, const QString &text, const QPointF &pos, const QFont &font);
    void drawSparkle(QPainter &painter, const QPointF &pos, qreal size, qreal rotation, const QColor &color);
    void drawCookieHalf(QPainter &painter, qreal xOffset, qreal rotation, bool isLeft, qreal scale);
    void drawLuckyNumberBalls(QPainter &painter, const QPointF &center, qreal y);
    void drawRetryButton(QPainter &painter, const QPointF &center, qreal y);
    void drawMagicButton(QPainter &painter, const QPointF &center, qreal y);
    
    void drawUnopenedCookie(QPainter &painter);
    void drawCrackingCookie(QPainter &painter);
    void drawOpenedFortune(QPainter &painter);
    
    QRectF getRetryButtonRect() const;
    QRectF getCookieRect() const;
    bool isPointInCookie(const QPointF &point) const;

private:
    CookieState m_state;
    FortuneData m_currentFortune;

    qreal m_rotation;
    qreal m_scale;
    qreal m_glowOpacity;
    qreal m_hoverScale;
    qreal m_crackProgress;
    qreal m_sparklePhase;
    QVector<QPointF> m_particlePositions;

    QTimer *m_animationTimer;
    QTimer *m_crackTimer;
    QPropertyAnimation *m_scaleAnimation;

    bool m_isHovered;
    bool m_isButtonHovered;
    bool m_isCookieHovered;
};

#endif // FORTUNECOOKIEWIDGET_H
