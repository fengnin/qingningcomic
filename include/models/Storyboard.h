
#pragma once

#include <QString>
#include <QVector>
#include <QMetaType>

/**
 * @brief 分镜数据模型
 * 对应原仓库 DynamoDB 中的 STORY 项
 */
class Storyboard
{
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString novelId READ novelId WRITE setNovelId)
    Q_PROPERTY(int chapterNumber READ chapterNumber WRITE setChapterNumber)
    Q_PROPERTY(int totalPages READ totalPages WRITE setTotalPages)
    Q_PROPERTY(int panelCount READ panelCount WRITE setPanelCount)
    Q_PROPERTY(int version READ version WRITE setVersion)

public:
    Storyboard();
    ~Storyboard() = default;

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString novelId() const { return m_novelId; }
    void setNovelId(const QString& novelId) { m_novelId = novelId; }

    int chapterNumber() const { return m_chapterNumber; }
    void setChapterNumber(int chapterNumber) { m_chapterNumber = chapterNumber; }

    int totalPages() const { return m_totalPages; }
    void setTotalPages(int totalPages) { m_totalPages = totalPages; }

    int panelCount() const { return m_panelCount; }
    void setPanelCount(int panelCount) { m_panelCount = panelCount; }

    int version() const { return m_version; }
    void setVersion(int version) { m_version = version; }

private:
    QString m_id;
    QString m_novelId;
    int m_chapterNumber = 1;
    int m_totalPages = 0;
    int m_panelCount = 0;
    int m_version = 1;
};

Q_DECLARE_METATYPE(Storyboard)
