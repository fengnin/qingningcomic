#ifndef IMAGEVIEWERDIALOG_H
#define IMAGEVIEWERDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QPixmap>
#include <QWheelEvent>
#include <QMouseEvent>

class ImageViewerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerDialog(QWidget *parent = nullptr);
    ~ImageViewerDialog();
    
    void setImage(const QString &imagePath);
    void setImage(const QPixmap &pixmap);
    
    static void showImage(QWidget *parent, const QString &imagePath);
    static void showImage(QWidget *parent, const QPixmap &pixmap);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void rotateLeft();
    void rotateRight();
    void closeDialog();

private:
    void setupUI();
    void setupConnections();
    void updateImage();
    void updateZoomLabel();
    bool loadFromNetworkUrl(const QString &url, QPixmap &outPixmap);
    bool loadFromLocalFile(const QString &path, QPixmap &outPixmap);
    
    QScrollArea *m_scrollArea;
    QLabel *m_imageLabel;
    QPushButton *m_zoomInBtn;
    QPushButton *m_zoomOutBtn;
    QPushButton *m_resetBtn;
    QPushButton *m_rotateLeftBtn;
    QPushButton *m_rotateRightBtn;
    QPushButton *m_closeBtn;
    QLabel *m_zoomLabel;
    
    QPixmap m_originalPixmap;
    double m_scaleFactor;
    int m_rotation;
    
    bool m_dragging;
    QPoint m_lastPos;
    
    static constexpr double MIN_SCALE = 0.1;
    static constexpr double MAX_SCALE = 10.0;
    static constexpr double SCALE_STEP = 0.1;
};

#endif // IMAGEVIEWERDIALOG_H
