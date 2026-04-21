#ifndef NOVELUPLOADUTILS_H
#define NOVELUPLOADUTILS_H

#include <QString>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>

namespace NovelUploadUtils {

namespace Validation {
    struct ValidationResult {
        bool isValid;
        QString errorMessage;
        
        ValidationResult(bool valid = true, const QString& error = QString())
            : isValid(valid), errorMessage(error) {}
    };
    
    inline ValidationResult checkFormData(const QString& title, const QString& text, int maxTextLength)
    {
        if (title.isEmpty() || text.isEmpty()) {
            return ValidationResult(false, QString::fromUtf8("请填写完整的小说标题与正文内容。"));
        }
        
        if (text.length() > maxTextLength) {
            return ValidationResult(false, QString::fromUtf8("单章字数请控制在 %1 字以内。").arg(maxTextLength));
        }
        
        return ValidationResult(true);
    }
    
    inline bool checkInputValid(const QString& title, const QString& text, int maxTextLength)
    {
        return !title.trimmed().isEmpty() &&
               !text.trimmed().isEmpty() &&
               text.length() <= maxTextLength;
    }
}

namespace State {
    enum class UploadState {
        Idle,
        Uploading,
        WaitingAnalysis,
        AnalysisCompleted,
        AnalysisFailed
    };
    
    inline void applyButtonState(QPushButton* button, bool enabled, const QString& text)
    {
        if (button) {
            button->setEnabled(enabled);
            button->setText(text);
        }
    }
    
    inline void applyProgressState(QProgressBar* progressBar, int value, bool visible)
    {
        if (progressBar) {
            progressBar->setValue(value);
            progressBar->setVisible(visible);
        }
    }
    
    inline void applyWaitingAnalysisState(QPushButton* uploadBtn, QProgressBar* progressBar, QLabel* successLabel)
    {
        applyButtonState(uploadBtn, false, QString::fromUtf8("正在分析小说中..."));
        applyProgressState(progressBar, 0, true);
        
        if (successLabel) {
            successLabel->setText(QString::fromUtf8("正在分析小说中，请等待完成..."));
            successLabel->setVisible(true);
        }
    }
    
    inline void applyResetAfterAnalysis(QPushButton* uploadBtn, QProgressBar* progressBar)
    {
        applyButtonState(uploadBtn, true, QString::fromUtf8("✅ 上传小说"));
        applyProgressState(progressBar, 0, false);
    }
    
    inline void applyLoadingState(QPushButton* uploadBtn, bool loading, bool isEditMode)
    {
        QString text = loading ?
            (isEditMode ? QString::fromUtf8("保存中...") : QString::fromUtf8("上传中...")) :
            (isEditMode ? QString::fromUtf8("保存修改") : QString::fromUtf8("✅ 上传小说"));
        applyButtonState(uploadBtn, !loading, text);
    }
}

namespace Message {
    inline void displayMessage(QLabel* errorLabel, QLabel* successLabel, const QString& message, bool isError)
    {
        if (errorLabel) errorLabel->setVisible(false);
        if (successLabel) successLabel->setVisible(false);
        
        QLabel* label = isError ? errorLabel : successLabel;
        if (label) {
            label->setText((isError ? "❌ " : "✅ ") + message);
            label->setVisible(true);
        }
    }
    
    inline void displayError(QLabel* errorLabel, QLabel* successLabel, const QString& message)
    {
        displayMessage(errorLabel, successLabel, message, true);
    }
    
    inline void displaySuccess(QLabel* errorLabel, QLabel* successLabel, const QString& message)
    {
        displayMessage(errorLabel, successLabel, message, false);
    }
    
    inline void clearAllMessages(QLabel* errorLabel, QLabel* successLabel)
    {
        if (errorLabel) errorLabel->setVisible(false);
        if (successLabel) successLabel->setVisible(false);
    }
    
    inline QString successMessageText(bool isEditMode)
    {
        return isEditMode ? QString::fromUtf8("小说更新成功！") : QString::fromUtf8("小说上传成功！");
    }
    
    inline QString errorMessageText(bool isEditMode)
    {
        return isEditMode ? QString::fromUtf8("保存失败，请检查数据库连接") : QString::fromUtf8("上传失败，请检查数据库连接");
    }
}

namespace Metadata {
    inline QVariantMap buildNovelMetadata(const QString& genre, int wordCount)
    {
        QVariantMap metadata;
        if (!genre.trimmed().isEmpty()) {
            metadata["genre"] = genre.trimmed();
        }
        metadata["word_count"] = wordCount;
        return metadata;
    }
}

}

#endif // NOVELUPLOADUTILS_H
