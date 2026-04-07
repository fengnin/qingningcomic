#include <QtTest/QtTest>
#include "utils/StatusHelper.h"

class TestStatusHelper : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testNovelLabel();
    void testNovelLabelUnknown();
    void testNovelStyle();
    void testNovelStyleUnknown();
    
    void testJobTypeLabel();
    void testJobTypeLabelUnknown();
    void testJobTypeLabelEmpty();
    void testJobStatusLabel();
    void testJobStatusLabelUnknown();
    void testJobStatusLabelEmpty();
    
    void testChapterLabel();
    void testChapterLabelUnknown();
    void testChapterStyle();
    void testChapterStyleUnknown();
};

void TestStatusHelper::initTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  StatusHelper 单元测试开始";
    qDebug() << "========================================";
}

void TestStatusHelper::cleanupTestCase()
{
    qDebug() << "========================================";
    qDebug() << "  StatusHelper 单元测试结束";
    qDebug() << "========================================";
}

void TestStatusHelper::testNovelLabel()
{
    QCOMPARE(StatusHelper::Novel::label("created"), QString::fromUtf8("已创建"));
    QCOMPARE(StatusHelper::Novel::label("analyzing"), QString::fromUtf8("分析中"));
    QCOMPARE(StatusHelper::Novel::label("analyzed"), QString::fromUtf8("已分析"));
    QCOMPARE(StatusHelper::Novel::label("generating"), QString::fromUtf8("生成中"));
    QCOMPARE(StatusHelper::Novel::label("completed"), QString::fromUtf8("已完成"));
    QCOMPARE(StatusHelper::Novel::label("error"), QString::fromUtf8("出错"));
}

void TestStatusHelper::testNovelLabelUnknown()
{
    QCOMPARE(StatusHelper::Novel::label("unknown_status"), QString::fromUtf8("未知"));
    QCOMPARE(StatusHelper::Novel::label("random"), QString::fromUtf8("未知"));
}

void TestStatusHelper::testNovelStyle()
{
    StatusHelper::StatusStyle style = StatusHelper::Novel::style("created");
    QCOMPARE(style.text, QString::fromUtf8("已创建"));
    QCOMPARE(style.bgColor, QString("#4dc7d2fe"));
    QCOMPARE(style.textColor, QString("#4f46e5"));
    
    style = StatusHelper::Novel::style("error");
    QCOMPARE(style.text, QString::fromUtf8("出错"));
    QCOMPARE(style.bgColor, QString("#3def4444"));
    QCOMPARE(style.textColor, QString("#dc2626"));
}

void TestStatusHelper::testNovelStyleUnknown()
{
    StatusHelper::StatusStyle style = StatusHelper::Novel::style("unknown");
    QCOMPARE(style.text, QString::fromUtf8("未知"));
    QCOMPARE(style.bgColor, QString("#336b7280"));
    QCOMPARE(style.textColor, QString("#6b7280"));
}

void TestStatusHelper::testJobTypeLabel()
{
    QCOMPARE(StatusHelper::Job::typeLabel("analyze"), QString::fromUtf8("分析小说"));
    QCOMPARE(StatusHelper::Job::typeLabel("generate_storyboard"), QString::fromUtf8("分析小说"));
    QCOMPARE(StatusHelper::Job::typeLabel("generate_preview"), QString::fromUtf8("面板预览"));
    QCOMPARE(StatusHelper::Job::typeLabel("generate_hd"), QString::fromUtf8("面板高清"));
    QCOMPARE(StatusHelper::Job::typeLabel("change_request"), QString::fromUtf8("修改请求"));
    QCOMPARE(StatusHelper::Job::typeLabel("panel_edit"), QString::fromUtf8("面板编辑"));
    QCOMPARE(StatusHelper::Job::typeLabel("generate_portrait"), QString::fromUtf8("角色标准像"));
    QCOMPARE(StatusHelper::Job::typeLabel("export_pdf"), QString::fromUtf8("导出 PDF"));
    QCOMPARE(StatusHelper::Job::typeLabel("export_webtoon"), QString::fromUtf8("导出 Webtoon"));
    QCOMPARE(StatusHelper::Job::typeLabel("export_resources"), QString::fromUtf8("导出资源包"));
}

void TestStatusHelper::testJobTypeLabelUnknown()
{
    QCOMPARE(StatusHelper::Job::typeLabel("unknown_type"), QString::fromUtf8("未知任务"));
}

void TestStatusHelper::testJobTypeLabelEmpty()
{
    QCOMPARE(StatusHelper::Job::typeLabel(""), QString::fromUtf8("未知任务"));
}

void TestStatusHelper::testJobStatusLabel()
{
    QCOMPARE(StatusHelper::Job::statusLabel("pending"), QString::fromUtf8("排队中"));
    QCOMPARE(StatusHelper::Job::statusLabel("in_progress"), QString::fromUtf8("进行中"));
    QCOMPARE(StatusHelper::Job::statusLabel("running"), QString::fromUtf8("进行中"));
    QCOMPARE(StatusHelper::Job::statusLabel("processing"), QString::fromUtf8("生成中"));
    QCOMPARE(StatusHelper::Job::statusLabel("completed"), QString::fromUtf8("已完成"));
    QCOMPARE(StatusHelper::Job::statusLabel("failed"), QString::fromUtf8("失败"));
}

void TestStatusHelper::testJobStatusLabelUnknown()
{
    QCOMPARE(StatusHelper::Job::statusLabel("unknown"), QString::fromUtf8("未知状态"));
}

void TestStatusHelper::testJobStatusLabelEmpty()
{
    QCOMPARE(StatusHelper::Job::statusLabel(""), QString::fromUtf8("未知状态"));
}

void TestStatusHelper::testChapterLabel()
{
    QCOMPARE(StatusHelper::Chapter::label("created"), QString::fromUtf8("未开始"));
    QCOMPARE(StatusHelper::Chapter::label("analyzing"), QString::fromUtf8("分析中"));
    QCOMPARE(StatusHelper::Chapter::label("completed"), QString::fromUtf8("已完成"));
    QCOMPARE(StatusHelper::Chapter::label("error"), QString::fromUtf8("出错"));
}

void TestStatusHelper::testChapterLabelUnknown()
{
    QCOMPARE(StatusHelper::Chapter::label("unknown"), QString::fromUtf8("未知"));
}

void TestStatusHelper::testChapterStyle()
{
    StatusHelper::StatusStyle style = StatusHelper::Chapter::style("created");
    QCOMPARE(style.text, QString::fromUtf8("未开始"));
    QCOMPARE(style.bgColor, QString("#E3F2FD"));
    QCOMPARE(style.textColor, QString("#1976D2"));
    
    style = StatusHelper::Chapter::style("completed");
    QCOMPARE(style.text, QString::fromUtf8("已完成"));
    QCOMPARE(style.bgColor, QString("#E8F5E9"));
    QCOMPARE(style.textColor, QString("#2E7D32"));
}

void TestStatusHelper::testChapterStyleUnknown()
{
    StatusHelper::StatusStyle style = StatusHelper::Chapter::style("unknown");
    QCOMPARE(style.text, QString::fromUtf8("未知"));
    QCOMPARE(style.bgColor, QString("#F5F5F5"));
    QCOMPARE(style.textColor, QString("#666666"));
}

QTEST_APPLESS_MAIN(TestStatusHelper)
#include "tst_statushelper.moc"
