#ifndef VOLCENGINEREQUEST_H
#define VOLCENGINEREQUEST_H

#include <QString>
#include <QJsonObject>
#include <QByteArray>
#include <QDateTime>

/**
 * @brief 火山引擎图像生成请求封装类
 * 
 * 封装文生图 API 的请求参数，提供便捷的构建方法。
 * 
 * API 文档：https://www.volcengine.com/docs/85128/1526761
 */
class VolcEngineRequest
{
public:
    /**
     * @brief 预设分辨率
     */
    enum class Resolution {
        Square_1x1,      // 1328x1328 (1:1)
        Standard_4x3,    // 1472x1104 (4:3)
        Standard_3x2,    // 1584x1056 (3:2)
        Widescreen_16x9, // 1664x936 (16:9)
        Ultrawide_21x9   // 2016x864 (21:9)
    };

    /**
     * @brief 构造函数
     * @param reqKey 模型标识，默认为 Seedream 3.0 文生图
     */
    explicit VolcEngineRequest(const QString& reqKey = "high_aes_general_v30l_zt2i");

    // ========== 必填参数 ==========

    /**
     * @brief 设置提示词
     * @param prompt 图像描述，中英文均可
     */
    VolcEngineRequest& setPrompt(const QString& prompt);

    // ========== 可选参数 ==========

    /**
     * @brief 设置分辨率（使用预设）
     * @param resolution 预设分辨率
     */
    VolcEngineRequest& setResolution(Resolution resolution);

    /**
     * @brief 设置分辨率（自定义）
     * @param width 宽度 [512, 2048]
     * @param height 高度 [512, 2048]
     */
    VolcEngineRequest& setResolution(int width, int height);

    /**
     * @brief 设置随机种子
     * @param seed 种子值，-1 表示随机
     */
    VolcEngineRequest& setSeed(int seed);

    /**
     * @brief 设置文本影响程度
     * @param scale 影响程度 [1.0, 10.0]
     */
    VolcEngineRequest& setScale(float scale);

    /**
     * @brief 设置是否返回 URL
     * @param returnUrl true 返回 URL，false 返回 Base64
     */
    VolcEngineRequest& setReturnUrl(bool returnUrl);

    /**
     * @brief 设置是否开启文本扩写
     * @param usePreLlm true 开启，prompt 较短时建议开启
     */
    VolcEngineRequest& setUsePreLlm(bool usePreLlm);

    /**
     * @brief 设置请求 ID
     * @param requestId 请求标识
     */
    VolcEngineRequest& setRequestId(const QString& requestId);

    // ========== 获取参数 ==========

    QString prompt() const { return m_prompt; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    int seed() const { return m_seed; }
    float scale() const { return m_scale; }
    bool returnUrl() const { return m_returnUrl; }
    bool usePreLlm() const { return m_usePreLlm; }
    QString requestId() const { return m_requestId; }

    // ========== 序列化 ==========

    /**
     * @brief 转换为 JSON 请求体
     */
    QJsonObject toJson() const;

    /**
     * @brief 转换为 JSON 字节数组
     */
    QByteArray toJsonBytes() const;

    // ========== 静态工厂方法 ==========

    /**
     * @brief 创建角色肖像请求
     * @param description 角色描述
     * @param requestId 请求 ID
     */
    static VolcEngineRequest forCharacterPortrait(const QString& description, 
                                                   const QString& requestId = QString());

    /**
     * @brief 创建场景参考图请求
     * @param description 场景描述
     * @param requestId 请求 ID
     */
    static VolcEngineRequest forSceneReference(const QString& description,
                                                const QString& requestId = QString());

    /**
     * @brief 创建漫画面板请求
     * @param description 面板描述
     * @param requestId 请求 ID
     */
    static VolcEngineRequest forPanel(const QString& description,
                                       const QString& requestId = QString());

private:
    QString m_reqKey;
    QString m_prompt;
    int m_width = 1328;
    int m_height = 1328;
    int m_seed = -1;
    float m_scale = 2.5f;
    bool m_returnUrl = true;
    bool m_usePreLlm = false;
    QString m_requestId;

    void getResolutionSize(Resolution resolution, int& width, int& height);
};

#endif // VOLCENGINEREQUEST_H
