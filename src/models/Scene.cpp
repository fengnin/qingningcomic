#include "models/Scene.h"
#include "utils/BibleUtils.h"

namespace {
QString translateSceneStatus(const QString& status)
{
    static const QMap<QString, QString> map = {
        {"unknown", QString::fromUtf8("未知")},
        {"inferred", QString::fromUtf8("推断中")},
        {"confirmed", QString::fromUtf8("已确认")},
        {"merged", QString::fromUtf8("已合并")}
    };
    return map.value(status, status);
}

QString translateConfidence(const QString& confidence)
{
    static const QMap<QString, QString> map = {
        {"low", QString::fromUtf8("低")},
        {"medium", QString::fromUtf8("中")},
        {"high", QString::fromUtf8("高")}
    };
    return map.value(confidence, confidence);
}
}

QStringList SceneDetails::toDisplayStrings() const
{
    using namespace BibleUtils::Inference;
    
    QStringList lines;
    
    auto addLine = [&lines](const QString& label, const QString& value) {
        if (!value.isEmpty()) {
            lines << QString::fromUtf8("%1: %2").arg(label, value);
        }
    };
    
    // 基础信息
    addLine(QString::fromUtf8("类型"), preferZh(typeZh, type, translateSceneType));
    addLine(QString::fromUtf8("描述"), description);

    // 视觉概览（聚合展示）
    QStringList visualBits;
    if (!building.isEmpty()) visualBits << building;
    if (!landmark.isEmpty()) visualBits << landmark;
    if (!color.isEmpty()) visualBits << color;
    if (!layout.isEmpty()) visualBits << layout;
    if (!atmosphere.isEmpty()) visualBits << atmosphere;
    if (!visualBits.isEmpty()) {
        addLine(QString::fromUtf8("视觉概览"), visualBits.join(QString::fromUtf8("；")));
    }

    // 视觉要素（逐项展示）
    addLine(QString::fromUtf8("建筑"), building);
    addLine(QString::fromUtf8("色调"), color);
    addLine(QString::fromUtf8("地标"), landmark);
    addLine(QString::fromUtf8("布局"), layout);
    addLine(QString::fromUtf8("氛围"), atmosphere);

    // 环境参数
    addLine(QString::fromUtf8("背景设定"), setting);
    if (!isIndoorSceneType(type)) {
        addLine(QString::fromUtf8("天气"), preferZh(QString(), weather, translateWeather));
    }
    addLine(QString::fromUtf8("时间段"), preferZh(QString(), timeOfDay, translateTimeOfDay));
    addLine(QString::fromUtf8("叙事角色"), preferZh(narrativeRoleZh, narrativeRole, translateNarrativeRole));

    // 环境条件（聚合展示）
    QStringList structureBits;
    if (!setting.isEmpty()) structureBits << setting;
    if (!timeOfDay.isEmpty()) structureBits << preferZh(QString(), timeOfDay, translateTimeOfDay);
    if (!isIndoorSceneType(type) && !weather.isEmpty()) {
        structureBits << preferZh(QString(), weather, translateWeather);
    }
    if (!structureBits.isEmpty()) {
        addLine(QString::fromUtf8("环境条件"), structureBits.join(QString::fromUtf8("；")));
    }

    // AI元数据（测试用，显示所有字段）
    if (!currentInterpretation.isEmpty()) {
        addLine(QString::fromUtf8("当前解释"), currentInterpretation);
    }

    QStringList iterationBits;
    if (!status.isEmpty()) iterationBits << QString::fromUtf8("状态: %1").arg(translateSceneStatus(status));
    if (!confidence.isEmpty()) iterationBits << QString::fromUtf8("置信度: %1").arg(translateConfidence(confidence));
    if (!iterationBits.isEmpty()) {
        addLine(QString::fromUtf8("迭代"), iterationBits.join(QString::fromUtf8("；")));
    }

    if (!evidence.isEmpty()) {
        addLine(QString::fromUtf8("证据"), evidence.join(QString::fromUtf8("；")));
    }

    if (!aliases.isEmpty()) {
        addLine(QString::fromUtf8("别名"), aliases.join(QString::fromUtf8("；")));
    }

    QStringList anchorBits;
    if (!anchorPoints.isEmpty()) anchorBits << anchorPoints.join(QString::fromUtf8("；"));
    if (!signatureObjects.isEmpty()) anchorBits << signatureObjects.join(QString::fromUtf8("；"));
    if (!fixedColorBlocks.isEmpty()) anchorBits << fixedColorBlocks.join(QString::fromUtf8("；"));
    if (!anchorBits.isEmpty()) {
        addLine(QString::fromUtf8("锚点"), anchorBits.join(QString::fromUtf8("；")));
    }

    if (!consistencyRules.isEmpty()) {
        addLine(QString::fromUtf8("一致性"), consistencyRules.join(QString::fromUtf8("；")));
    }

    if (!history.isEmpty()) {
        addLine(QString::fromUtf8("历史"), history.join(QString::fromUtf8("；")));
    }

    return lines;
}
