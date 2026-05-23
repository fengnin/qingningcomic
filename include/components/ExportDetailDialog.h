#ifndef EXPORTDETAILDIALOG_H
#define EXPORTDETAILDIALOG_H

#include "services/ExportService.h"
#include <QDialog>
#include <QLabel>
#include <QPushButton>

class ExportDetailDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDetailDialog(QWidget *parent = nullptr);

    void setData(const ExportResult &result);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void setupUI();
    QWidget* createDetailItem(const QString &labelText, QLabel **valueLabel);

    QLabel *m_exportIdLabel = nullptr;
    QLabel *m_novelIdLabel = nullptr;
    QLabel *m_formatLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_fileSizeLabel = nullptr;
    QLabel *m_createdAtLabel = nullptr;
    QLabel *m_fileUrlLabel = nullptr;
    QPushButton *m_openBtn = nullptr;
    QPushButton *m_closeBtn = nullptr;

    void animateFadeIn();
    void animateFadeOut();
};

#endif // EXPORTDETAILDIALOG_H
