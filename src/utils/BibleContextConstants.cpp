#include "utils/BibleUtils.h"

namespace BibleUtils {
namespace BibleContextConstants {

const QStringList& getDynamicTokens()
{
    static const QStringList tokens = {
        QString::fromUtf8("我"),
        QString::fromUtf8("你"),
        QString::fromUtf8("他"),
        QString::fromUtf8("她"),
        QString::fromUtf8("他们"),
        QString::fromUtf8("她们"),
        QString::fromUtf8("角色"),
        QString::fromUtf8("人物"),
        QString::fromUtf8("主角"),
        QString::fromUtf8("配角"),
        QString::fromUtf8("说"),
        QString::fromUtf8("喊"),
        QString::fromUtf8("问"),
        QString::fromUtf8("看"),
        QString::fromUtf8("走"),
        QString::fromUtf8("跑"),
        QString::fromUtf8("站"),
        QString::fromUtf8("坐"),
        QString::fromUtf8("躺"),
        QString::fromUtf8("笑"),
        QString::fromUtf8("哭"),
        QString::fromUtf8("目光"),
        QString::fromUtf8("视线"),
        QString::fromUtf8("指尖"),
        QString::fromUtf8("声音"),
        QString::fromUtf8("对话"),
        QString::fromUtf8("镜头"),
        QString::fromUtf8("特写"),
        QString::fromUtf8("近景"),
        QString::fromUtf8("中景"),
        QString::fromUtf8("远景")
    };
    return tokens;
}

const QStringList& getLocationHints()
{
    static const QStringList hints = {
        QString::fromUtf8("房"),
        QString::fromUtf8("室"),
        QString::fromUtf8("厅"),
        QString::fromUtf8("厨房"),
        QString::fromUtf8("客厅"),
        QString::fromUtf8("卧室"),
        QString::fromUtf8("走廊"),
        QString::fromUtf8("门"),
        QString::fromUtf8("院"),
        QString::fromUtf8("楼"),
        QString::fromUtf8("街"),
        QString::fromUtf8("路"),
        QString::fromUtf8("巷"),
        QString::fromUtf8("店"),
        QString::fromUtf8("摊"),
        QString::fromUtf8("市场"),
        QString::fromUtf8("广场"),
        QString::fromUtf8("车站"),
        QString::fromUtf8("码头"),
        QString::fromUtf8("剧场"),
        QString::fromUtf8("戏台"),
        QString::fromUtf8("后台"),
        QString::fromUtf8("仓库"),
        QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"),
        QString::fromUtf8("病房"),
        QString::fromUtf8("阳台"),
        QString::fromUtf8("天台"),
        QString::fromUtf8("屋顶"),
        QString::fromUtf8("庭院"),
        QString::fromUtf8("入口"),
        QString::fromUtf8("门口")
    };
    return hints;
}

const QStringList& getLocationSuffixes()
{
    static const QStringList suffixes = {
        QString::fromUtf8("客厅"),
        QString::fromUtf8("厨房"),
        QString::fromUtf8("卧室"),
        QString::fromUtf8("书房"),
        QString::fromUtf8("教室"),
        QString::fromUtf8("办公室"),
        QString::fromUtf8("病房"),
        QString::fromUtf8("走廊"),
        QString::fromUtf8("院门"),
        QString::fromUtf8("门口"),
        QString::fromUtf8("门前"),
        QString::fromUtf8("门内"),
        QString::fromUtf8("院子"),
        QString::fromUtf8("庭院"),
        QString::fromUtf8("阳台"),
        QString::fromUtf8("天台"),
        QString::fromUtf8("屋顶"),
        QString::fromUtf8("楼道"),
        QString::fromUtf8("街道"),
        QString::fromUtf8("小径"),
        QString::fromUtf8("小巷"),
        QString::fromUtf8("石板路"),
        QString::fromUtf8("书市"),
        QString::fromUtf8("旧书市"),
        QString::fromUtf8("摊位"),
        QString::fromUtf8("摊前"),
        QString::fromUtf8("店铺"),
        QString::fromUtf8("市场"),
        QString::fromUtf8("广场"),
        QString::fromUtf8("后台"),
        QString::fromUtf8("舞台"),
        QString::fromUtf8("戏台"),
        QString::fromUtf8("剧场"),
        QString::fromUtf8("仓库")
    };
    return suffixes;
}

} // namespace BibleContextConstants
} // namespace BibleUtils
