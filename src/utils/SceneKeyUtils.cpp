#include "utils/SceneKeyUtils.h"

namespace SceneKeyUtils {

QString stripSceneModifiers(QString text)
{
    static const QStringList leadingTokens = {
        QString::fromUtf8("老式"),
        QString::fromUtf8("老旧"),
        QString::fromUtf8("破旧"),
        QString::fromUtf8("破败"),
        QString::fromUtf8("陈旧"),
        QString::fromUtf8("昏暗"),
        QString::fromUtf8("昏黄"),
        QString::fromUtf8("阴暗"),
        QString::fromUtf8("阴冷"),
        QString::fromUtf8("冷清"),
        QString::fromUtf8("空荡"),
        QString::fromUtf8("空旷"),
        QString::fromUtf8("狭窄"),
        QString::fromUtf8("宽敞"),
        QString::fromUtf8("简陋"),
        QString::fromUtf8("整洁"),
        QString::fromUtf8("凌乱"),
        QString::fromUtf8("潮湿"),
        QString::fromUtf8("废弃"),
        QString::fromUtf8("荒芜"),
        QString::fromUtf8("静谧"),
        QString::fromUtf8("安静"),
        QString::fromUtf8("寂静"),
        QString::fromUtf8("繁忙"),
        QString::fromUtf8("喧闹"),
        QString::fromUtf8("拥挤"),
        QString::fromUtf8("明亮"),
        QString::fromUtf8("清冷"),
        QString::fromUtf8("雨中"),
        QString::fromUtf8("雪中"),
        QString::fromUtf8("风中"),
        QString::fromUtf8("夜色中"),
        QString::fromUtf8("灯下"),
        QString::fromUtf8("黑暗中"),
        QString::fromUtf8("暴雨"),
        QString::fromUtf8("大雨"),
        QString::fromUtf8("小雨"),
        QString::fromUtf8("倾盆"),
        QString::fromUtf8("狂风"),
        QString::fromUtf8("细雨"),
        QString::fromUtf8("阴雨"),
        QString::fromUtf8("晴朗"),
        QString::fromUtf8("阴沉")
    };

    bool changed = true;
    while (changed) {
        changed = false;
        for (const QString& token : leadingTokens) {
            if (text.startsWith(token)) {
                text = text.mid(token.length()).trimmed();
                changed = true;
                break;
            }
        }
        text.remove(QRegularExpression(QStringLiteral(R"(^[：:\s]+)")));
    }
    return text.trimmed();
}

QString normalizeSceneLabel(QString text)
{
    QString result = text.trimmed();
    if (result.isEmpty()) {
        return result;
    }

    static const QStringList prefixes = {
        QString::fromUtf8("场景"),
        QString::fromUtf8("背景"),
        QString::fromUtf8("环境"),
        QStringLiteral("scene"),
        QStringLiteral("setting"),
        QStringLiteral("location")
    };

    for (const QString& prefix : prefixes) {
        if (result.startsWith(prefix, Qt::CaseInsensitive)) {
            QString stripped = result.mid(prefix.length()).trimmed();
            stripped.remove(QRegularExpression(QStringLiteral(R"(^[：:\s]+)")));
            if (!stripped.isEmpty()) {
                result = stripped;
            }
            break;
        }
    }

    const int punctuationPos = result.indexOf(QRegularExpression(QStringLiteral(R"([：:；;。！？!?，,、\n\r])")));
    if (punctuationPos > 0) {
        result = result.left(punctuationPos).trimmed();
    }

    static const QStringList shotSuffixes = {
        QString::fromUtf8("全景"),
        QString::fromUtf8("特写"),
        QString::fromUtf8("近景"),
        QString::fromUtf8("中景"),
        QString::fromUtf8("远景"),
        QString::fromUtf8("大远景"),
        QString::fromUtf8("大全景"),
        QString::fromUtf8("俯视"),
        QString::fromUtf8("仰视"),
        QString::fromUtf8("鸟瞰"),
        QString::fromUtf8("侧面")
    };

    for (const QString& suffix : shotSuffixes) {
        if (result.endsWith(suffix) && result.length() > suffix.length()) {
            result.chop(suffix.length());
            result = result.trimmed();
            break;
        }
    }

    static const QStringList locationSuffixes = {
        QString::fromUtf8("内景"),
        QString::fromUtf8("外景"),
        QString::fromUtf8("入口处"),
        QString::fromUtf8("出口处"),
        QString::fromUtf8("门口"),
        QString::fromUtf8("门外"),
        QString::fromUtf8("门前"),
        QString::fromUtf8("门内"),
        QString::fromUtf8("门后"),
        QString::fromUtf8("入口"),
        QString::fromUtf8("出口"),
        QString::fromUtf8("柜台"),
        QString::fromUtf8("前台"),
        QString::fromUtf8("书架区"),
        QString::fromUtf8("阅读区"),
        QString::fromUtf8("休息区"),
        QString::fromUtf8("展示区"),
        QString::fromUtf8("等候区"),
        QString::fromUtf8("接待区"),
        QString::fromUtf8("候场区"),
        QString::fromUtf8("一角"),
        QString::fromUtf8("一隅"),
        QString::fromUtf8("深处"),
        QString::fromUtf8("角落"),
        QString::fromUtf8("内部"),
        QString::fromUtf8("外部"),
        QString::fromUtf8("旁边"),
        QString::fromUtf8("附近"),
        QString::fromUtf8("周围"),
        QString::fromUtf8("中央"),
        QString::fromUtf8("中心")
    };

    bool locationStripped = true;
    while (locationStripped) {
        locationStripped = false;
        for (const QString& suffix : locationSuffixes) {
            if (result.endsWith(suffix)) {
                const QString stripped = result.left(result.length() - suffix.length()).trimmed();
                if (stripped.length() >= 2) {
                    result = stripped;
                    locationStripped = true;
                    break;
                }
            }
        }
    }

    return stripSceneModifiers(result);
}

QString extractSceneNounKey(const QString& text)
{
    static const QStringList nouns = {
        QString::fromUtf8("外廊"),
        QString::fromUtf8("客厅"),
        QString::fromUtf8("厨房"),
        QString::fromUtf8("卧室"),
        QString::fromUtf8("走廊"),
        QString::fromUtf8("玄关"),
        QString::fromUtf8("门口"),
        QString::fromUtf8("门廊"),
        QString::fromUtf8("门前"),
        QString::fromUtf8("楼道"),
        QString::fromUtf8("楼梯"),
        QString::fromUtf8("楼层"),
        QString::fromUtf8("阳台"),
        QString::fromUtf8("天台"),
        QString::fromUtf8("院子"),
        QString::fromUtf8("庭院"),
        QString::fromUtf8("后院"),
        QString::fromUtf8("前院"),
        QString::fromUtf8("街道"),
        QString::fromUtf8("小路"),
        QString::fromUtf8("小径"),
        QString::fromUtf8("荒径"),
        QString::fromUtf8("山路"),
        QString::fromUtf8("巷子"),
        QString::fromUtf8("巷道"),
        QString::fromUtf8("公路"),
        QString::fromUtf8("马路"),
        QString::fromUtf8("广场"),
        QString::fromUtf8("操场"),
        QString::fromUtf8("球场"),
        QString::fromUtf8("停车场"),
        QString::fromUtf8("停车位"),
        QString::fromUtf8("地下室"),
        QString::fromUtf8("地下车库"),
        QString::fromUtf8("地下通道"),
        QString::fromUtf8("地铁站"),
        QString::fromUtf8("地铁"),
        QString::fromUtf8("公交站"),
        QString::fromUtf8("公交车"),
        QString::fromUtf8("火车站"),
        QString::fromUtf8("火车"),
        QString::fromUtf8("机场"),
        QString::fromUtf8("港口"),
        QString::fromUtf8("码头"),
        QString::fromUtf8("渡口"),
        QString::fromUtf8("桥上"),
        QString::fromUtf8("桥下"),
        QString::fromUtf8("桥头"),
        QString::fromUtf8("隧道"),
        QString::fromUtf8("涵洞"),
        QString::fromUtf8("天桥"),
        QString::fromUtf8("立交桥"),
        QString::fromUtf8("高架桥"),
        QString::fromUtf8("公园"),
        QString::fromUtf8("花园"),
        QString::fromUtf8("菜园"),
        QString::fromUtf8("果园"),
        QString::fromUtf8("农田"),
        QString::fromUtf8("田野"),
        QString::fromUtf8("草地"),
        QString::fromUtf8("草坪"),
        QString::fromUtf8("林地"),
        QString::fromUtf8("树林"),
        QString::fromUtf8("森林"),
        QString::fromUtf8("山顶"),
        QString::fromUtf8("山腰"),
        QString::fromUtf8("山脚"),
        QString::fromUtf8("山谷"),
        QString::fromUtf8("山洞"),
        QString::fromUtf8("洞穴"),
        QString::fromUtf8("悬崖"),
        QString::fromUtf8("峭壁"),
        QString::fromUtf8("海边"),
        QString::fromUtf8("海滩"),
        QString::fromUtf8("沙滩"),
        QString::fromUtf8("湖边"),
        QString::fromUtf8("湖畔"),
        QString::fromUtf8("河边"),
        QString::fromUtf8("河岸"),
        QString::fromUtf8("溪边"),
        QString::fromUtf8("瀑布"),
        QString::fromUtf8("池塘"),
        QString::fromUtf8("水库"),
        QString::fromUtf8("井边"),
        QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"),
        QString::fromUtf8("会议室"),
        QString::fromUtf8("实验室"),
        QString::fromUtf8("图书馆"),
        QString::fromUtf8("阅览室"),
        QString::fromUtf8("档案室"),
        QString::fromUtf8("储藏室"),
        QString::fromUtf8("机房"),
        QString::fromUtf8("医院"),
        QString::fromUtf8("诊室"),
        QString::fromUtf8("病房"),
        QString::fromUtf8("手术室"),
        QString::fromUtf8("急诊室"),
        QString::fromUtf8("药房"),
        QString::fromUtf8("餐厅"),
        QString::fromUtf8("食堂"),
        QString::fromUtf8("酒吧"),
        QString::fromUtf8("咖啡厅"),
        QString::fromUtf8("茶室"),
        QString::fromUtf8("包厢"),
        QString::fromUtf8("大堂"),
        QString::fromUtf8("车站"),
        QString::fromUtf8("站台"),
        QString::fromUtf8("剧场"),
        QString::fromUtf8("舞台"),
        QString::fromUtf8("后台"),
        QString::fromUtf8("前台"),
        QString::fromUtf8("看台"),
        QString::fromUtf8("观众席"),
        QString::fromUtf8("休息区"),
        QString::fromUtf8("候场区"),
        QString::fromUtf8("商店"),
        QString::fromUtf8("店铺"),
        QString::fromUtf8("仓库"),
        QString::fromUtf8("大厅"),
        QString::fromUtf8("前厅"),
        QString::fromUtf8("房间"),
        QString::fromUtf8("屋内"),
        QString::fromUtf8("屋外"),
        QString::fromUtf8("住宅"),
        QString::fromUtf8("居民楼"),
        QString::fromUtf8("单元楼"),
        QString::fromUtf8("入口"),
        QString::fromUtf8("出口"),
        QString::fromUtf8("转角"),
        QString::fromUtf8("交界处")
    };

    int bestPos = -1;
    QString bestKey;
    for (const QString& noun : nouns) {
        const int pos = text.lastIndexOf(noun);
        if (pos < 0) {
            continue;
        }
        if (pos > bestPos || (pos == bestPos && noun.length() > bestKey.length())) {
            bestPos = pos;
            bestKey = text.mid(pos, noun.length());
        }
    }

    return bestKey;
}

QString buildSceneCoreKey(const QString& text)
{
    QString result = text.trimmed();
    if (result.isEmpty()) {
        return result;
    }

    const int punctuationPos = result.indexOf(QRegularExpression(QStringLiteral(R"([：:；;。！？!?，,、\n\r])")));
    if (punctuationPos > 0) {
        result = result.left(punctuationPos).trimmed();
    }

    result = stripSceneModifiers(result);
    if (result.isEmpty()) {
        return result;
    }

    const QString nounKey = extractSceneNounKey(result);
    if (!nounKey.isEmpty()) {
        return nounKey;
    }

    static const QStringList abstractTokens = {
        QString::fromUtf8("背景"),
        QString::fromUtf8("画面"),
        QString::fromUtf8("投影"),
        QString::fromUtf8("记忆"),
        QString::fromUtf8("幻象"),
        QString::fromUtf8("闪回"),
        QString::fromUtf8("镜头"),
        QString::fromUtf8("特写"),
        QString::fromUtf8("全景"),
        QString::fromUtf8("近景"),
        QString::fromUtf8("中景"),
        QString::fromUtf8("远景")
    };

    if (containsAnyToken(result, abstractTokens)) {
        return QString();
    }

    return result;
}

} // namespace SceneKeyUtils
