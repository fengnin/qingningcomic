
#pragma once

#include <QString>
#include <QStringList>
#include <QMetaType>
#include <QVariantMap>
#include <QJsonObject>

/**
 * @brief 角色外观描述
 * 包含角色的外貌特征细节
 */
struct CharacterAppearance
{
    QString gender;          // 性别：male, female, other
    int age = 0;             // 年龄
    QString hairColor;       // 发色
    QString hairStyle;       // 发型
    QString eyeColor;        // 瞳色
    QString height;          // 身高描述
    QString build;           // 体型描述
    QStringList clothing;    // 服装列表
    QStringList accessories; // 配饰列表
    QStringList distinctiveFeatures;  // 显著特征
    QStringList aliases;     // 历史别名 / 关联称呼
    QJsonObject fieldSources; // 字段来源：explicit / inferred / manual
    
    QJsonObject toJson() const;
    QJsonObject toPromptJson() const;  // prompt 用的精简格式，不含 fieldSources/aliases
    static CharacterAppearance fromJson(const QJsonObject& json);
    
    // 判断外观是否有空字段
    inline bool hasEmptyFields() const
    {
        return gender.isEmpty() || age == 0 || hairColor.isEmpty() ||
               hairStyle.isEmpty() || eyeColor.isEmpty() || build.isEmpty() ||
               aliases.isEmpty();
    }
};

/**
 * @brief 角色数据模型
 * 存储角色圣经 (Character Bible) 信息
 */
class Character
{
    Q_GADGET
    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QString novelId READ novelId WRITE setNovelId)
    Q_PROPERTY(QString name READ name WRITE setName)
    Q_PROPERTY(QString role READ role WRITE setRole)
    Q_PROPERTY(CharacterAppearance appearance READ appearance WRITE setAppearance)
    Q_PROPERTY(QStringList personality READ personality WRITE setPersonality)
    Q_PROPERTY(QStringList aliases READ aliases WRITE setAliases)
    Q_PROPERTY(QString portraitPath READ portraitPath WRITE setPortraitPath)
    Q_PROPERTY(QString currentPortraitVersionId READ currentPortraitVersionId WRITE setCurrentPortraitVersionId)

public:
    /**
     * @brief 构造函数
     */
    Character();

    /**
     * @brief 析构函数
     */
    ~Character() = default;

    // ==================== Getter 和 Setter 方法 ====================

    QString id() const { return m_id; }
    void setId(const QString& id) { m_id = id; }

    QString novelId() const { return m_novelId; }
    void setNovelId(const QString& novelId) { m_novelId = novelId; }

    QString name() const { return m_name; }
    void setName(const QString& name) { m_name = name; }

    QString role() const { return m_role; }
    void setRole(const QString& role) { m_role = role; }

    CharacterAppearance appearance() const { return m_appearance; }
    void setAppearance(const CharacterAppearance& appearance)
    {
        m_appearance = appearance;
        m_aliases = appearance.aliases;
    }

    QStringList personality() const { return m_personality; }
    void setPersonality(const QStringList& personality) { m_personality = personality; }

    QStringList aliases() const { return m_aliases.isEmpty() ? m_appearance.aliases : m_aliases; }
    void setAliases(const QStringList& aliases)
    {
        m_aliases = aliases;
        m_appearance.aliases = aliases;
    }

    QString portraitPath() const { return m_portraitPath; }
    void setPortraitPath(const QString& path) { m_portraitPath = path; }

    QString currentPortraitVersionId() const { return m_currentPortraitVersionId; }
    void setCurrentPortraitVersionId(const QString& id) { m_currentPortraitVersionId = id; }

    // 序列化方法
    QVariantMap toVariantMap() const;
    static Character fromVariantMap(const QVariantMap& map);
    QJsonObject toJson() const;
    static Character fromJson(const QJsonObject& json);
    
    // 显示格式
    QStringList toDisplayStrings() const;

private:
    QString m_id;              // 角色唯一ID，对应 SK: CHAR#{id}
    QString m_novelId;         // 所属小说ID，对应 PK: NOVEL#{novelId}
    QString m_name;           // 角色名称
    QString m_role;           // 角色角色：protagonist, secondary, antagonist 等
    CharacterAppearance m_appearance;  // 外观描述
    QStringList m_personality;  // 性格特征列表
    QStringList m_aliases;      // 历史别名 / 关联称呼
    QString m_portraitPath;    // 肖像图片路径（始终镜像 current_portrait_version 的 portrait_path）
    QString m_currentPortraitVersionId; // 当前活跃肖像版本 id，指向 character_portrait_versions
};

/**
 * @brief 角色配置数据模型
 * 角色的不同外观配置（如：战斗模式、日常模式等）
 */
struct CharacterConfiguration
{
    QString id;                     // 配置ID
    QString charId;                 // 所属角色ID
    QString novelId;                // 所属作品ID
    QString name;                   // 配置名称
    QString description;            // 配置描述
    QStringList tags;               // 标签列表
    QStringList referenceImages;    // 参考图URL列表
    QStringList portraits;          // 生成的标准像URL列表
    bool isDefault = false;         // 是否为默认配置
};

// 声明元类型
Q_DECLARE_METATYPE(Character)
Q_DECLARE_METATYPE(CharacterAppearance)
Q_DECLARE_METATYPE(CharacterConfiguration)
