#ifndef STATUSHELPER_H
#define STATUSHELPER_H

#include <QString>
#include <QMap>

namespace StatusHelper {

struct StatusStyle {
    QString text;
    QString bgColor;
    QString textColor;
};

namespace Novel {
    inline QString label(const QString& status) {
        static const QMap<QString, QString> labels = {
            {"created", QString::fromUtf8("已创建")},
            {"analyzing", QString::fromUtf8("分析中")},
            {"completed", QString::fromUtf8("已完成")},
            {"error", QString::fromUtf8("出错")}
        };
        return labels.value(status, QString::fromUtf8("未知"));
    }
    
    inline StatusStyle style(const QString& status) {
        static const QMap<QString, StatusStyle> styles = {
            {"created", {"已创建", "#4dc7d2fe", "#4f46e5"}},
            {"analyzing", {"分析中", "#3dd8b4f8", "#7c3aed"}},
            {"completed", {"已完成", "#3d86efac", "#166534"}},
            {"error", {"出错", "#3def4444", "#dc2626"}}
        };
        return styles.value(status, {"未知", "#336b7280", "#6b7280"});
    }
}

namespace Job {
    inline QString typeLabel(const QString& type) {
        static const QMap<QString, QString> labels = {
            {"analyze", QString::fromUtf8("分析小说")},
            {"generate_storyboard", QString::fromUtf8("分析小说")},
            {"GenerateStoryboard", QString::fromUtf8("分析小说")},
            {"generate_preview", QString::fromUtf8("面板预览生成")},
            {"generate_hd", QString::fromUtf8("面板高清生成")},
            {"GeneratePanelImage", QString::fromUtf8("面板高清生成")},
            {"generate_panel_image", QString::fromUtf8("面板高清生成")},
            {"GeneratePanels", QString::fromUtf8("面板预览生成")},
            {"generate_panels", QString::fromUtf8("面板预览生成")},
            {"change_request", QString::fromUtf8("修改请求执行")},
            {"panel_edit", QString::fromUtf8("面板编辑")},
            {"generate_portrait", QString::fromUtf8("角色标准像")},
            {"export_pdf", QString::fromUtf8("导出 PDF")},
            {"ExportPdf", QString::fromUtf8("导出 PDF")},
            {"export_webtoon", QString::fromUtf8("导出 Webtoon")},
            {"export_resources", QString::fromUtf8("导出资源包")}
        };
        if (type.isEmpty()) return QString::fromUtf8("未知任务");
        return labels.value(type, QString::fromUtf8("未知任务"));
    }
    
    inline QString statusLabel(const QString& status) {
        static const QMap<QString, QString> labels = {
            {"pending", QString::fromUtf8("排队中")},
            {"Pending", QString::fromUtf8("排队中")},
            {"in_progress", QString::fromUtf8("进行中")},
            {"running", QString::fromUtf8("进行中")},
            {"Running", QString::fromUtf8("进行中")},
            {"processing", QString::fromUtf8("生成中")},
            {"completed", QString::fromUtf8("已完成")},
            {"Completed", QString::fromUtf8("已完成")},
            {"failed", QString::fromUtf8("失败")},
            {"Failed", QString::fromUtf8("失败")},
            {"cancelled", QString::fromUtf8("已取消")},
            {"Cancelled", QString::fromUtf8("已取消")}
        };
        if (status.isEmpty()) return QString::fromUtf8("未知状态");
        return labels.value(status, QString::fromUtf8("未知状态"));
    }
}

namespace Chapter {
    inline QString label(const QString& status) {
        static const QMap<QString, QString> labels = {
            {"created", QString::fromUtf8("未开始")},
            {"analyzing", QString::fromUtf8("分析中")},
            {"completed", QString::fromUtf8("已完成")},
            {"generated", QString::fromUtf8("已生成")},
            {"processing", QString::fromUtf8("处理中")},
            {"error", QString::fromUtf8("出错")},
            {"failed", QString::fromUtf8("生成失败")}
        };
        return labels.value(status, QString::fromUtf8("待生成"));
    }
    
    inline StatusStyle style(const QString& status) {
        static const QMap<QString, StatusStyle> styles = {
            {"created", {"未开始", "#E3F2FD", "#1976D2"}},
            {"analyzing", {"分析中", "#FFF3E0", "#EF6C00"}},
            {"completed", {"已完成", "#E8F5E9", "#2E7D32"}},
            {"generated", {"已生成", "#E8F5E9", "#16A34A"}},
            {"processing", {"处理中", "#FFF3E0", "#D97706"}},
            {"error", {"出错", "#FFEBEE", "#DC2626"}},
            {"failed", {"生成失败", "#FFEBEE", "#DC2626"}}
        };
        return styles.value(status, {"待生成", "#F5F5F5", "#6B7280"});
    }
}

}

#endif // STATUSHELPER_H
