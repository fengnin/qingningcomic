#include "widgets/FortuneCookieWidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QFontMetrics>
#include <QDateTime>
#include <QtMath>
#include <QDebug>
#include <QMouseEvent>

namespace FortuneCookieStyle {
    constexpr int COOKIE_WIDTH = 128;
    constexpr int COOKIE_HEIGHT = 80;
    constexpr int WIDGET_MIN_HEIGHT = 380;
    constexpr int ANIMATION_INTERVAL = 33;
    constexpr int CRACK_DURATION = 2000;
    constexpr qreal SPARKLE_ROTATION_SPEED = 150.0;
    constexpr qreal SPARKLE_INTERNAL_SPEED = 45.0;
    
    namespace Color {
        const QColor BG_START(255, 251, 235);
        const QColor BG_END(254, 243, 199);
        const QColor COOKIE_LIGHT(254, 249, 195);
        const QColor COOKIE_MIDDLE(254, 240, 138);
        const QColor COOKIE_DARK(253, 224, 71);
        const QColor COOKIE_BORDER(250, 204, 21);
        const QColor COOKIE_TEXTURE(234, 179, 8, 50);
        const QColor TEXT_TITLE(180, 83, 9);
        const QColor TEXT_TITLE_LIGHT(202, 138, 4);
        const QColor TEXT_SUBTITLE(180, 83, 9);
        const QColor TEXT_QUOTE(55, 65, 81);
        const QColor TEXT_NUMBER(120, 53, 15);
        const QColor SPARKLE_ORANGE(251, 146, 60);
        const QColor SPARKLE_YELLOW(250, 204, 21);
        const QColor SPARKLE_GOLD(217, 119, 6);
        const QColor SPARKLE_RED(234, 88, 12);
        const QColor NUMBER_BG_START(254, 240, 138);
        const QColor NUMBER_BG_END(250, 204, 21);
    }

    namespace FortuneList {
        const QVector<QPair<QString, QVector<int>>> ALL = {
            {QStringLiteral("你今天的所作所为，塑造了你的未来，而非明天。"), {7, 14, 23, 31, 42, 56}},
            {QStringLiteral("种树最好的时机是二十年前，其次是现在。"), {3, 18, 27, 35, 49, 63}},
            {QStringLiteral("成功不是终点，失败也不是终局，重要的是继续前行的勇气。"), {9, 16, 24, 38, 47, 55}},
            {QStringLiteral("相信自己，你已经成功了一半。"), {2, 11, 29, 33, 44, 51}},
            {QStringLiteral("热爱你所做的事，这是成就伟大工作的唯一方式。"), {5, 12, 21, 36, 43, 58}},
            {QStringLiteral("生活，就发生在你忙于制定计划的时候。"), {1, 19, 26, 34, 41, 62}},
            {QStringLiteral("走得再慢，只要不停下，就没关系。"), {8, 15, 22, 37, 46, 59}},
            {QStringLiteral("困境之中，往往藏着机遇。"), {4, 13, 25, 32, 48, 57}},
            {QStringLiteral("千里之行，始于足下。"), {6, 17, 28, 39, 45, 61}},
            {QStringLiteral("我们身后和身前的，与我们内心深处的相比，都微不足道。"), {10, 20, 30, 40, 50, 60}},
            {QStringLiteral("做你自己，因为其他人都已经有人做了。"), {12, 24, 36, 41, 53, 65}},
            {QStringLiteral("林中两条路，我选了人迹罕至的那条，从此一切都不同了。"), {14, 28, 35, 49, 56, 63}},
            {QStringLiteral("心即一切，你所想，便成你所是。"), {1, 8, 15, 22, 29, 36}},
            {QStringLiteral("不管你觉得能还是不能，你都是对的。"), {3, 9, 18, 27, 45, 54}},
            {QStringLiteral("昨日已成历史，明日尚是谜，今天是上天的馈赠，所以叫做当下。"), {7, 21, 28, 42, 49, 56}},
            {QStringLiteral("不要用收获来评价每一天，而要用播下的种子来衡量。"), {11, 22, 33, 44, 55, 66}},
            {QStringLiteral("世上唯一不可能的旅程，是你从未启程的那一段。"), {2, 13, 24, 35, 46, 57}},
            {QStringLiteral("最终，我们记住的不是敌人的话，而是朋友的沉默。"), {16, 32, 48, 64, 17, 33}},
            {QStringLiteral("正是在最黑暗的时刻，我们最需要凝神望向那一束光。"), {5, 10, 25, 40, 55, 65}},
            {QStringLiteral("将爱散播到你所到之处，让每个人离开你时都比来时更快乐。"), {9, 18, 36, 45, 54, 63}},
            {QStringLiteral("走到绳子尽头时，打个结，抓紧它。"), {4, 8, 16, 32, 48, 52}},
            {QStringLiteral("不要让你做不到的事，妨碍你做得到的事。"), {6, 12, 24, 30, 42, 60}},
            {QStringLiteral("想要提升自己，先去提升他人。"), {15, 30, 45, 52, 67, 3}},
            {QStringLiteral("未来属于那些相信梦想之美的人。"), {19, 38, 57, 4, 23, 68}},
            {QStringLiteral("宁可在独创中失败，也不愿在模仿中成功。"), {23, 46, 69, 8, 17, 54}},
            {QStringLiteral("开始的方式，是停止空谈，开始行动。"), {27, 54, 12, 41, 65, 29}},
            {QStringLiteral("别害怕放弃好的，去追求更好的。"), {31, 62, 18, 47, 6, 35}},
            {QStringLiteral("创新，区分了领导者与跟随者。"), {35, 70, 14, 53, 9, 26}},
            {QStringLiteral("生命中最大的荣耀，不在于从不跌倒，而在于每次跌倒后都能重新站起。"), {39, 7, 28, 56, 11, 64}},
            {QStringLiteral("你的时间有限，不要把它浪费在过别人的生活上。"), {43, 16, 32, 59, 5, 48}},
            {QStringLiteral("如果生活是可以预料的，它就不再是生活，而是毫无滋味。"), {47, 24, 61, 13, 37, 52}},
            {QStringLiteral("生活其实很简单，是我们执意要把它变得复杂。"), {51, 2, 39, 66, 21, 44}},
            {QStringLiteral("此刻的你，已经拥有面对一切所需的一切。"), {55, 10, 43, 69, 25, 38}},
            {QStringLiteral("相信自己，知道你内心有某种力量，远超任何障碍。"), {59, 18, 47, 3, 29, 65}},
            {QStringLiteral("你注定要成为的那个人，就是你决定去成为的那个人。"), {63, 26, 51, 7, 33, 58}},
            {QStringLiteral("朝着梦想的方向坚定前行，活出你所想象的生活。"), {67, 34, 55, 11, 37, 62}},
            {QStringLiteral("幸福的一扇门关上时，另一扇会打开，但我们常常凝望那扇关闭的门，错过了为我们开启的那扇。"), {4, 42, 59, 15, 68, 27}},
            {QStringLiteral("你所渴望的一切，都在恐惧的另一端。"), {8, 46, 63, 19, 35, 52}},
            {QStringLiteral("心怀大梦，敢于失败。"), {12, 50, 67, 23, 39, 56}},
            {QStringLiteral("你不出手的那些机会，百分之百会错过。"), {16, 54, 2, 27, 43, 60}}
        };
    }
}

using namespace FortuneCookieStyle;
using namespace FortuneCookieStyle::Color;

FortuneCookieWidget::FortuneCookieWidget(QWidget *parent)
    : QWidget(parent)
    , m_state(CookieState::Unopened)
    , m_rotation(0.0)
    , m_scale(1.0)
    , m_glowOpacity(0.3)
    , m_hoverScale(1.0)
    , m_crackProgress(0.0)
    , m_sparklePhase(0.0)
    , m_animationTimer(nullptr)
    , m_crackTimer(nullptr)
    , m_scaleAnimation(nullptr)
    , m_isHovered(false)
    , m_isButtonHovered(false)
    , m_isCookieHovered(false)
{
    initFortunes();
    initAnimations();
    
    setMinimumHeight(WIDGET_MIN_HEIGHT);
    setMinimumWidth(220);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(true);
    setCursor(Qt::PointingHandCursor);
}

FortuneCookieWidget::~FortuneCookieWidget()
{
    if (m_animationTimer) {
        m_animationTimer->stop();
    }
    if (m_crackTimer) {
        m_crackTimer->stop();
    }
}

void FortuneCookieWidget::initFortunes()
{
}

void FortuneCookieWidget::initAnimations()
{
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, [this]() {
        m_sparklePhase += 0.05;
        if (m_sparklePhase > 2 * M_PI) {
            m_sparklePhase -= 2 * M_PI;
        }
        
        if (m_isHovered && m_state == CookieState::Unopened) {
            m_hoverScale = 1.05 + 0.03 * qSin(m_sparklePhase * 2);
        }
        
        // 在 Cracking 状态下更新进度
        if (m_state == CookieState::Cracking && m_crackProgress < 1.0) {
            m_crackProgress += 0.04;
            if (m_crackProgress > 1.0) {
                m_crackProgress = 1.0;
            }
        }
        
        update();
    });
    
    m_crackTimer = new QTimer(this);
    m_crackTimer->setSingleShot(true);
    connect(m_crackTimer, &QTimer::timeout, this, [this]() {
        m_state = CookieState::Opened;
        emit stateChanged(m_state);
        emit fortuneRevealed(m_currentFortune);
        updateAnimationTimer();
        update();
    });

    updateAnimationTimer();
}

FortuneData FortuneCookieWidget::getRandomFortune()
{
    const auto& fortunes = FortuneCookieStyle::FortuneList::ALL;
    if (fortunes.isEmpty()) {
        return {QStringLiteral("大吉大利，万事如意"), {1, 2, 3, 4, 5, 6}};
    }
    
    int index = QRandomGenerator::global()->bounded(fortunes.size());
    const auto& f = fortunes[index];
    return {f.first, f.second};
}

void FortuneCookieWidget::crackCookie()
{
    if (m_state != CookieState::Unopened) return;
    
    m_state = CookieState::Cracking;
    m_currentFortune = getRandomFortune();
    m_crackProgress = 0.0;
    
    // 预计算粒子位置，避免每帧生成随机数
    m_particlePositions.clear();
    for (int i = 0; i < 8; ++i) {
        qreal angle = (i * 2 * M_PI) / 8 + M_PI / 4;
        m_particlePositions.append(QPointF(qCos(angle), qSin(angle) * 0.5));
    }
    
    emit stateChanged(m_state);
    
    m_scaleAnimation = new QPropertyAnimation(this, "cookieScale");
    m_scaleAnimation->setDuration(500);
    m_scaleAnimation->setStartValue(1.0);
    m_scaleAnimation->setKeyValueAt(0.3, 1.1);
    m_scaleAnimation->setKeyValueAt(0.6, 0.9);
    m_scaleAnimation->setEndValue(1.0);
    connect(m_scaleAnimation, &QObject::destroyed, this, [this](QObject *) {
        m_scaleAnimation = nullptr;
    });
    m_scaleAnimation->start(QPropertyAnimation::DeleteWhenStopped);
    
    // 使用单一动画定时器，合并 m_animationTimer 和 progressTimer
    // m_animationTimer 已经在运行，只需要设置 m_crackTimer 来结束动画
    m_crackTimer->start(CRACK_DURATION);
    updateAnimationTimer();
}

void FortuneCookieWidget::resetCookie()
{
    m_state = CookieState::Unopened;
    m_crackProgress = 0.0;
    m_scale = 1.0;
    m_rotation = 0.0;
    m_currentFortune = FortuneData();
    
    emit stateChanged(m_state);
    updateAnimationTimer();
    update();
}

void FortuneCookieWidget::setCookieRotation(qreal rotation)
{
    if (!qFuzzyCompare(m_rotation, rotation)) {
        m_rotation = rotation;
        update();
    }
}

void FortuneCookieWidget::setCookieScale(qreal scale)
{
    if (!qFuzzyCompare(m_scale, scale)) {
        m_scale = scale;
        update();
    }
}

void FortuneCookieWidget::setGlowOpacity(qreal opacity)
{
    if (!qFuzzyCompare(m_glowOpacity, opacity)) {
        m_glowOpacity = opacity;
        update();
    }
}

void FortuneCookieWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    drawBackground(painter);
    
    switch (m_state) {
        case CookieState::Unopened:
            drawUnopenedCookie(painter);
            break;
        case CookieState::Cracking:
            drawCrackingCookie(painter);
            break;
        case CookieState::Opened:
            drawOpenedFortune(painter);
            break;
    }
}

void FortuneCookieWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_state == CookieState::Unopened) {
        if (isPointInCookie(event->pos())) {
            crackCookie();
        }
    } else if (m_state == CookieState::Opened) {
        QRectF btnRect = getRetryButtonRect();
        if (btnRect.contains(event->pos())) {
            resetCookie();
        }
    }
}

void FortuneCookieWidget::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = true;
    updateCursor();
    updateAnimationTimer();
}

void FortuneCookieWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_isHovered = false;
    m_hoverScale = 1.0;
    m_isButtonHovered = false;
    m_isCookieHovered = false;
    setCursor(Qt::ArrowCursor);
    updateAnimationTimer();
    update();
}

void FortuneCookieWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_state == CookieState::Unopened) {
        bool wasHovered = m_isCookieHovered;
        m_isCookieHovered = isPointInCookie(event->pos());
        
        if (m_isCookieHovered) {
            setCursor(Qt::PointingHandCursor);
            m_hoverScale = 1.05;
        } else {
            setCursor(Qt::ArrowCursor);
            m_hoverScale = 1.0;
        }
        
        if (wasHovered != m_isCookieHovered) {
            update();
        }
    } else if (m_state == CookieState::Opened) {
        QRectF btnRect = getRetryButtonRect();
        bool wasHovered = m_isButtonHovered;
        m_isButtonHovered = btnRect.contains(event->pos());
        
        if (m_isButtonHovered) {
            setCursor(Qt::PointingHandCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
        
        if (wasHovered != m_isButtonHovered) {
            update();
        }
    }
}

void FortuneCookieWidget::updateCursor()
{
    if (m_state == CookieState::Unopened) {
        setCursor(Qt::ArrowCursor);
    } else if (m_state == CookieState::Opened) {
        setCursor(Qt::ArrowCursor);
    }
}

void FortuneCookieWidget::updateAnimationTimer()
{
    // 始终运行动画，不只是 hover 时
    const bool shouldAnimate = true;

    if (shouldAnimate) {
        if (!m_animationTimer->isActive()) {
            m_animationTimer->start(ANIMATION_INTERVAL);
        }
    } else if (m_animationTimer->isActive()) {
        m_animationTimer->stop();
    }
}

void FortuneCookieWidget::drawBackground(QPainter &painter)
{
    QLinearGradient bgGradient(0, 0, 0, height());
    bgGradient.setColorAt(0, BG_START);
    bgGradient.setColorAt(1, BG_END);
    painter.fillRect(rect(), bgGradient);
}

void FortuneCookieWidget::drawGradientText(QPainter &painter, const QString &text, 
                                            const QPointF &pos, const QFont &font)
{
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(text);
    QPointF textPos(pos.x() - textWidth / 2, pos.y());
    
    QPainterPath textPath;
    textPath.addText(textPos, font, text);
    
    QLinearGradient textGradient(
        QPointF(textPos.x(), textPos.y() - 20),
        QPointF(textPos.x() + textWidth, textPos.y()));
    textGradient.setColorAt(0, TEXT_TITLE);
    textGradient.setColorAt(0.5, TEXT_TITLE_LIGHT);
    textGradient.setColorAt(1, TEXT_TITLE);
    
    painter.setBrush(textGradient);
    painter.setPen(Qt::NoPen);
    painter.drawPath(textPath);
}

void FortuneCookieWidget::drawSparkle(QPainter &painter, const QPointF &pos, 
                                       qreal size, qreal rotation, const QColor &color)
{
    painter.save();
    
    painter.translate(pos);
    painter.rotate(rotation + m_sparklePhase * SPARKLE_INTERNAL_SPEED);
    
    QPainterPath starPath;
    starPath.moveTo(0, -size);
    starPath.lineTo(size * 0.25, -size * 0.25);
    starPath.lineTo(size, 0);
    starPath.lineTo(size * 0.25, size * 0.25);
    starPath.lineTo(0, size);
    starPath.lineTo(-size * 0.25, size * 0.25);
    starPath.lineTo(-size, 0);
    starPath.lineTo(-size * 0.25, -size * 0.25);
    starPath.closeSubpath();
    
    QColor sparkleColor = color;
    sparkleColor.setAlpha(200);
    painter.setBrush(sparkleColor);
    painter.setPen(Qt::NoPen);
    painter.drawPath(starPath);
    
    QColor glowColor = color;
    glowColor.setAlpha(80);
    painter.setBrush(glowColor);
    QPainterPath glowPath;
    glowPath.addEllipse(QPointF(0, 0), size * 1.5, size * 1.5);
    painter.drawPath(glowPath);
    
    painter.restore();
}

void FortuneCookieWidget::drawCookieHalf(QPainter &painter, qreal xOffset, qreal rotation, 
                                          bool isLeft, qreal scale)
{
    painter.save();
    painter.translate(xOffset, 0);
    painter.rotate(rotation);
    painter.scale(scale, scale);
    
    qreal halfWidth = COOKIE_WIDTH / 2;
    QRectF halfRect(isLeft ? -COOKIE_WIDTH : 0, -COOKIE_HEIGHT / 2, halfWidth, COOKIE_HEIGHT);
    
    QLinearGradient gradient(halfRect.topLeft(), halfRect.bottomRight());
    gradient.setColorAt(0, COOKIE_LIGHT);
    gradient.setColorAt(1, COOKIE_DARK);
    
    painter.setBrush(gradient);
    painter.setPen(QPen(COOKIE_BORDER, 2));
    
    QPainterPath clipPath;
    clipPath.addEllipse(-halfWidth, -COOKIE_HEIGHT / 2, COOKIE_WIDTH, COOKIE_HEIGHT);
    
    QPainterPath halfPath;
    halfPath.addRect(isLeft ? -halfWidth : 0, -COOKIE_HEIGHT / 2, halfWidth, COOKIE_HEIGHT);
    halfPath = halfPath.intersected(clipPath);
    painter.drawPath(halfPath);
    
    painter.setPen(QPen(COOKIE_TEXTURE, 1));
    painter.setBrush(Qt::NoBrush);
    
    QPainterPath innerClip;
    innerClip.addEllipse(-halfWidth + 8, -COOKIE_HEIGHT / 2 + 6, COOKIE_WIDTH - 16, COOKIE_HEIGHT - 12);
    
    QPainterPath innerPath;
    qreal innerX = isLeft ? -halfWidth + 8 : 8;
    innerPath.addRect(innerX, -COOKIE_HEIGHT / 2 + 6, halfWidth - 16, COOKIE_HEIGHT - 12);
    innerPath = innerPath.intersected(innerClip);
    painter.drawPath(innerPath);
    
    painter.restore();
}

void FortuneCookieWidget::drawLuckyNumberBalls(QPainter &painter, const QPointF &center, qreal y)
{
    if (m_currentFortune.luckyNumbers.isEmpty()) return;
    
    int numberCount = qMin(6, m_currentFortune.luckyNumbers.size());
    qreal numberSize = 32;
    qreal totalWidth = numberCount * numberSize + (numberCount - 1) * 8;
    qreal startX = center.x() - totalWidth / 2;
    
    for (int i = 0; i < numberCount; ++i) {
        qreal x = startX + i * (numberSize + 8);
        
        QRadialGradient numGradient(QPointF(x + numberSize / 2, y), numberSize / 2);
        numGradient.setColorAt(0, NUMBER_BG_START);
        numGradient.setColorAt(1, NUMBER_BG_END);
        
        painter.setBrush(numGradient);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x + numberSize / 2, y), numberSize / 2, numberSize / 2);
        
        QFont numFont(QStringLiteral("Arial"), 11, QFont::Bold);
        painter.setFont(numFont);
        painter.setPen(TEXT_NUMBER);
        
        QString numStr = QString::number(m_currentFortune.luckyNumbers[i]);
        QFontMetrics numFm(numFont);
        int numWidth = numFm.horizontalAdvance(numStr);
        painter.drawText(QPointF(x + numberSize / 2 - numWidth / 2, y + 4), numStr);
    }
}

void FortuneCookieWidget::drawRetryButton(QPainter &painter, const QPointF &center, qreal y)
{
    QString btnText = QString::fromUtf8("\u518d\u6765\u4e00\u6b21");
    QFont btnFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Medium);
    QFontMetrics btnFm(btnFont);
    int btnTextWidth = btnFm.horizontalAdvance(btnText);
    
    qreal btnW = btnTextWidth + 36;
    qreal btnH = 30;
    QRectF btnRect(center.x() - btnW / 2, y - btnH / 2, btnW, btnH);
    
    QLinearGradient btnGrad(btnRect.topLeft(), btnRect.bottomRight());
    if (m_isButtonHovered) {
        btnGrad.setColorAt(0, BG_START);
        btnGrad.setColorAt(1, COOKIE_MIDDLE);
        painter.setPen(QPen(TEXT_TITLE, 1.5));
    } else {
        btnGrad.setColorAt(0, COOKIE_MIDDLE);
        btnGrad.setColorAt(1, COOKIE_BORDER);
        painter.setPen(Qt::NoPen);
    }
    
    painter.setBrush(btnGrad);
    painter.drawRoundedRect(btnRect, btnH / 2, btnH / 2);
    
    painter.setFont(btnFont);
    painter.setPen(m_isButtonHovered ? TEXT_TITLE : TEXT_NUMBER);
    painter.drawText(QPointF(center.x() - btnTextWidth / 2, y + 4), btnText);
}

void FortuneCookieWidget::drawUnopenedCookie(QPainter &painter)
{
    QRectF widgetRect = rect();
    qreal widgetHeight = widgetRect.height();
    
    qreal cookieY = widgetHeight * 0.28;
    qreal titleY = widgetHeight * 0.58;
    qreal subtitleY = widgetHeight * 0.72;
    qreal buttonY = widgetHeight * 0.86;
    
    QPointF center = widgetRect.center();
    center.setY(cookieY);
    
    qreal currentScale = m_scale * m_hoverScale;
    qreal floatY = qSin(m_sparklePhase * 2) * 4;
    qreal wobble = qSin(m_sparklePhase) * 2;
    
    painter.save();
    painter.translate(center.x(), center.y() + floatY);
    painter.scale(currentScale, currentScale);
    painter.rotate(12 + m_rotation + wobble);
    
    QPainterPath shadowPath;
    shadowPath.addEllipse(QPointF(6, 8), COOKIE_WIDTH / 2, COOKIE_HEIGHT / 2);
    QColor shadowColor(0, 0, 0, 50);
    painter.setBrush(shadowColor);
    painter.setPen(Qt::NoPen);
    painter.drawPath(shadowPath);
    
    QRectF cookieRect(-COOKIE_WIDTH / 2, -COOKIE_HEIGHT / 2, COOKIE_WIDTH, COOKIE_HEIGHT);
    QLinearGradient cookieGradient(cookieRect.topLeft(), cookieRect.bottomRight());
    cookieGradient.setColorAt(0, COOKIE_LIGHT);
    cookieGradient.setColorAt(0.5, COOKIE_MIDDLE);
    cookieGradient.setColorAt(1, COOKIE_DARK);
    
    painter.setBrush(cookieGradient);
    painter.setPen(QPen(COOKIE_BORDER, 2));
    painter.drawEllipse(cookieRect);
    
    painter.setPen(QPen(COOKIE_TEXTURE, 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(cookieRect.adjusted(8, 6, -8, -6));
    painter.drawEllipse(cookieRect.adjusted(16, 12, -16, -12));
    
    painter.restore();
    
    drawSparkle(painter, QPointF(center.x() + COOKIE_WIDTH / 2 + 5, center.y() - COOKIE_HEIGHT / 2 - 5),
                14, m_sparklePhase * SPARKLE_ROTATION_SPEED, SPARKLE_YELLOW);
    
    drawSparkle(painter, QPointF(center.x() - COOKIE_WIDTH / 2 - 8, center.y() + COOKIE_HEIGHT / 2 + 2),
                10, -m_sparklePhase * (SPARKLE_ROTATION_SPEED + SPARKLE_INTERNAL_SPEED), SPARKLE_ORANGE);
    
    QFont titleFont(QStringLiteral("Microsoft YaHei"), 20, QFont::Bold);
    drawGradientText(painter, QStringLiteral("幸运饼干"), QPointF(center.x(), titleY), titleFont);
    
    QFont subtitleFont(QStringLiteral("Microsoft YaHei"), 11);
    painter.setFont(subtitleFont);
    painter.setPen(TEXT_SUBTITLE);
    
    QString subtitle = QStringLiteral("点击饼干，开启好运！");
    QFontMetrics fm(subtitleFont);
    int textWidth = fm.horizontalAdvance(subtitle);
    painter.drawText(QPointF(center.x() - textWidth / 2, subtitleY), subtitle);
    
    drawMagicButton(painter, center, buttonY);
}

void FortuneCookieWidget::drawCrackingCookie(QPainter &painter)
{
    QRectF widgetRect = rect();
    qreal widgetHeight = widgetRect.height();
    
    QPointF center = widgetRect.center();
    center.setY(widgetHeight * 0.38);
    
    qreal halfWidth = COOKIE_WIDTH / 2;
    qreal splitOffset = m_crackProgress * 28;
    qreal leftRotate = m_crackProgress * (-15);
    qreal rightRotate = m_crackProgress * 15;
    qreal shakeScale = 1.0 + qSin(m_crackProgress * 15) * 0.05 * (1 - m_crackProgress);
    
    painter.save();
    painter.translate(center);
    
    drawCookieHalf(painter, -halfWidth / 2 - splitOffset, leftRotate, true, shakeScale);
    drawCookieHalf(painter, halfWidth / 2 + splitOffset, rightRotate, false, shakeScale);
    
    // 使用预计算的粒子位置，避免每帧生成随机数
    int particleCount = qMin(static_cast<int>(8 * m_crackProgress), m_particlePositions.size());
    for (int i = 0; i < particleCount; ++i) {
        qreal distance = 10 + m_crackProgress * 35;
        qreal x = m_particlePositions[i].x() * distance;
        qreal y = m_particlePositions[i].y() * distance;
        
        qreal size = 2 + 2 * (1 - m_crackProgress * 0.5);
        QColor particleColor = SPARKLE_YELLOW;
        particleColor.setAlpha(static_cast<int>(200 * (1 - m_crackProgress * 0.7)));
        
        painter.setBrush(particleColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QPointF(x, y), size, size);
    }
    
    if (m_crackProgress > 0.3) {
        int sparkleCount = static_cast<int>(4 * (m_crackProgress - 0.3) / 0.7);
        for (int i = 0; i < sparkleCount; ++i) {
            qreal angle = (i * M_PI) / sparkleCount + m_sparklePhase;
            qreal distance = 20 + m_crackProgress * 30;
            qreal x = qCos(angle) * distance;
            qreal y = qSin(angle) * distance * 0.4;
            
            drawSparkle(painter, QPointF(x, y), 4, m_sparklePhase * SPARKLE_ROTATION_SPEED, SPARKLE_YELLOW);
        }
    }
    
    drawSparkle(painter, QPointF(-halfWidth - 15, -COOKIE_HEIGHT / 2 - 15),
                12, m_sparklePhase * SPARKLE_ROTATION_SPEED, SPARKLE_ORANGE);
    
    drawSparkle(painter, QPointF(halfWidth + 15, -COOKIE_HEIGHT / 2 - 15),
                12, -m_sparklePhase * (SPARKLE_ROTATION_SPEED + SPARKLE_INTERNAL_SPEED), SPARKLE_YELLOW);
    
    painter.restore();
    
    QFont loadingFont(QStringLiteral("Microsoft YaHei"), 11);
    painter.setFont(loadingFont);
    painter.setPen(TEXT_SUBTITLE);
    
    QString loadingText = QString::fromUtf8("\u6b63\u5728\u4e3a\u60a8\u62bd\u53d6\u8fd0\u52bf...");
    QFontMetrics fm(loadingFont);
    int textWidth = fm.horizontalAdvance(loadingText);
    painter.drawText(QPointF(center.x() - textWidth / 2, widgetHeight * 0.72), loadingText);
}

void FortuneCookieWidget::drawOpenedFortune(QPainter &painter)
{
    QRectF widgetRect = rect();
    qreal widgetHeight = widgetRect.height();
    QPointF center = widgetRect.center();
    
    qreal sparkleCenterY = widgetHeight * 0.14;
    
    qreal ringPulse = 0.8 + 0.7 * qAbs(qSin(m_sparklePhase * 2));
    int ringAlpha = static_cast<int>(80 * (1.5 - ringPulse));
    if (ringAlpha > 0) {
        painter.setPen(QPen(QColor(251, 191, 36, ringAlpha), 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QPointF(center.x(), sparkleCenterY), 28 * ringPulse, 28 * ringPulse);
    }
    
    drawSparkle(painter, QPointF(center.x(), sparkleCenterY),
                24, m_sparklePhase * SPARKLE_ROTATION_SPEED, SPARKLE_GOLD);
    
    struct SparklePos {
        qreal xPercent;
        qreal yPercent;
        qreal size;
        QColor color;
        qreal rotationOffset;
    };
    
    const QVector<SparklePos> sparkles = {
        { -0.35, 0.04,   6,  SPARKLE_ORANGE,  -80 },
        {  0.38,  0.06,   7,  SPARKLE_YELLOW,   70 },
        { -0.22, 0.16,   5,  QColor(234, 179, 8),    90 },
        {  0.28, 0.17,   5,  SPARKLE_ORANGE,  -60 },
        { -0.42, 0.12,   4,  SPARKLE_YELLOW,  120 },
        {  0.45, 0.13,   4,  QColor(234, 179, 8),  -120 },
    };
    
    for (const auto &sp : sparkles) {
        qreal floatX = qSin(m_sparklePhase + sp.rotationOffset / 30) * 3;
        qreal floatY = qCos(m_sparklePhase * 1.5 + sp.rotationOffset / 40) * 2;
        
        drawSparkle(painter,
            QPointF(center.x() + center.x() * sp.xPercent + floatX,
                     widgetHeight * sp.yPercent + floatY),
            sp.size,
            m_sparklePhase * SPARKLE_ROTATION_SPEED,
            sp.color);
    }
    
    QFont titleFont(QStringLiteral("Microsoft YaHei"), 18, QFont::Bold);
    drawGradientText(painter, QString::fromUtf8("\u4eca\u65e5\u8fd0\u52bf"), 
                     QPointF(center.x(), widgetHeight * 0.30), titleFont);
    
    QFont quoteFont(QStringLiteral("Georgia"), 11);
    quoteFont.setItalic(true);
    painter.setFont(quoteFont);
    painter.setPen(TEXT_QUOTE);
    
    QString quote = m_currentFortune.quote;
    QFontMetrics quoteFm(quoteFont);
    int availableWidth = static_cast<int>(widgetRect.width() - 40);
    int charWidth = qMax(1, quoteFm.horizontalAdvance(QStringLiteral("中")));
    int charsPerLine = qMax(8, availableWidth / charWidth);
    
    QStringList lines;
    for (int i = 0; i < quote.length(); i += charsPerLine) {
        lines << quote.mid(i, charsPerLine);
    }
    
    qreal lineHeight = 20;
    qreal quoteStartY = widgetHeight * 0.42;
    int maxLines = qMin(lines.size(), 3);
    
    QFont quoteMarkFont(QStringLiteral("Georgia"), 16, QFont::Normal);
    painter.setFont(quoteMarkFont);
    painter.setPen(QColor(180, 83, 9, 150));
    
    if (!lines.isEmpty()) {
        int firstLineWidth = quoteFm.horizontalAdvance(lines[0]);
        painter.drawText(QPointF(center.x() - firstLineWidth / 2 - 18, quoteStartY + 2), QStringLiteral("\""));
        
        int lastLineIdx = qMin(maxLines - 1, lines.size() - 1);
        int lastLineWidth = quoteFm.horizontalAdvance(lines[lastLineIdx]);
        painter.drawText(QPointF(center.x() + lastLineWidth / 2 + 4, quoteStartY + (maxLines - 1) * lineHeight + 2), QStringLiteral("\""));
    }
    
    painter.setFont(quoteFont);
    painter.setPen(TEXT_QUOTE);
    for (int i = 0; i < maxLines; ++i) {
        int lineWidth = quoteFm.horizontalAdvance(lines[i]);
        painter.drawText(QPointF(center.x() - lineWidth / 2, quoteStartY + i * lineHeight), lines[i]);
    }
    
    QFont luckyTitleFont(QStringLiteral("Microsoft YaHei"), 9);
    painter.setFont(luckyTitleFont);
    painter.setPen(TEXT_SUBTITLE);
    
    QString luckyTitle = QString::fromUtf8("\u5e78\u8fd0\u6570\u5b57");
    QFontMetrics luckyTitleFm(luckyTitleFont);
    int luckyTitleWidth = luckyTitleFm.horizontalAdvance(luckyTitle);
    painter.drawText(QPointF(center.x() - luckyTitleWidth / 2, widgetHeight * 0.72), luckyTitle);
    
    drawLuckyNumberBalls(painter, center, widgetHeight * 0.82);
    
    drawRetryButton(painter, center, widgetHeight * 0.94);
}

void FortuneCookieWidget::drawMagicButton(QPainter &painter, const QPointF &center, qreal y)
{
    painter.save();
    
    QString buttonText = QStringLiteral("Magic waiting button");
    QFont btnFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Medium);
    QFontMetrics fm(btnFont);
    int textWidth = fm.horizontalAdvance(buttonText);
    
    qreal btnWidth = textWidth + 32;
    qreal btnHeight = 28;
    qreal btnX = center.x() - btnWidth / 2;
    
    QRectF btnRect(btnX, y - btnHeight / 2, btnWidth, btnHeight);
    
    int borderAlpha = static_cast<int>(50 + 100 * qAbs(qSin(m_sparklePhase * 3)));
    
    QLinearGradient btnGradient(btnRect.topLeft(), btnRect.bottomRight());
    btnGradient.setColorAt(0, QColor(255, 251, 235, 200));
    btnGradient.setColorAt(1, QColor(254, 243, 199, 200));
    
    painter.setBrush(btnGradient);
    painter.setPen(QPen(QColor(253, 230, 138, borderAlpha), 1.5));
    painter.drawRoundedRect(btnRect, btnHeight / 2, btnHeight / 2);
    
    drawSparkle(painter, QPointF(btnX + 14, y),
                7, m_sparklePhase * SPARKLE_ROTATION_SPEED, SPARKLE_GOLD);
    
    drawSparkle(painter, QPointF(btnX + btnWidth - 14, y),
                7, -m_sparklePhase * (SPARKLE_ROTATION_SPEED + SPARKLE_INTERNAL_SPEED), SPARKLE_RED);
    
    painter.setPen(TEXT_SUBTITLE);
    painter.setFont(btnFont);
    painter.drawText(QPointF(center.x() - textWidth / 2, y + 4), buttonText);
    
    painter.restore();
}

QRectF FortuneCookieWidget::getRetryButtonRect() const
{
    QRectF widgetRect = rect();
    qreal widgetHeight = widgetRect.height();
    QPointF center = widgetRect.center();
    
    QString btnText = QString::fromUtf8("\u518d\u62bd\u4e00\u6b21");
    QFont btnFont(QStringLiteral("Microsoft YaHei"), 10, QFont::Medium);
    QFontMetrics btnFm(btnFont);
    int btnTextWidth = btnFm.horizontalAdvance(btnText);
    
    qreal btnW = btnTextWidth + 36;
    qreal btnH = 30;
    qreal btnY = widgetHeight * 0.94;
    
    return QRectF(center.x() - btnW / 2, btnY - btnH / 2, btnW, btnH);
}

QRectF FortuneCookieWidget::getCookieRect() const
{
    QRectF widgetRect = rect();
    qreal widgetHeight = widgetRect.height();
    
    qreal cookieY = widgetHeight * 0.28;
    
    QPointF center = widgetRect.center();
    center.setY(cookieY);
    
    return QRectF(center.x() - COOKIE_WIDTH / 2,
                  center.y() - COOKIE_HEIGHT / 2,
                  COOKIE_WIDTH,
                  COOKIE_HEIGHT);
}

bool FortuneCookieWidget::isPointInCookie(const QPointF &point) const
{
    QRectF cookieRect = getCookieRect();
    qreal cx = cookieRect.center().x();
    qreal cy = cookieRect.center().y();
    qreal rx = cookieRect.width() / 2;
    qreal ry = cookieRect.height() / 2;
    
    qreal dx = (point.x() - cx) / rx;
    qreal dy = (point.y() - cy) / ry;
    
    return (dx * dx + dy * dy) <= 1.0;
}
