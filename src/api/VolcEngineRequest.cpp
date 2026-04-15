#include "api/VolcEngineRequest.h"
#include <QJsonDocument>

VolcEngineRequest::VolcEngineRequest(const QString& reqKey)
    : m_reqKey(reqKey)
{
}

VolcEngineRequest& VolcEngineRequest::setPrompt(const QString& prompt)
{
    m_prompt = prompt;
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setResolution(Resolution resolution)
{
    getResolutionSize(resolution, m_width, m_height);
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setResolution(int width, int height)
{
    m_width = qBound(512, width, 2048);
    m_height = qBound(512, height, 2048);
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setSeed(int seed)
{
    m_seed = seed;
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setScale(float scale)
{
    m_scale = qBound(1.0f, scale, 10.0f);
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setReturnUrl(bool returnUrl)
{
    m_returnUrl = returnUrl;
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setUsePreLlm(bool usePreLlm)
{
    m_usePreLlm = usePreLlm;
    return *this;
}

VolcEngineRequest& VolcEngineRequest::setRequestId(const QString& requestId)
{
    m_requestId = requestId;
    return *this;
}

QJsonObject VolcEngineRequest::toJson() const
{
    QJsonObject json;
    json["req_key"] = m_reqKey;
    json["prompt"] = m_prompt;
    json["width"] = m_width;
    json["height"] = m_height;
    json["scale"] = m_scale;
    json["return_url"] = m_returnUrl;
    
    if (m_seed >= 0) {
        json["seed"] = m_seed;
    }
    
    if (m_usePreLlm) {
        json["use_pre_llm"] = true;
    }
    
    return json;
}

QByteArray VolcEngineRequest::toJsonBytes() const
{
    return QJsonDocument(toJson()).toJson(QJsonDocument::Compact);
}

VolcEngineRequest VolcEngineRequest::forCharacterPortrait(const QString& description,
                                                           const QString& requestId)
{
    return VolcEngineRequest()
        .setPrompt(description)
        .setResolution(Resolution::Square_1x1)
        .setReturnUrl(true)
        .setRequestId(requestId);
}

VolcEngineRequest VolcEngineRequest::forSceneReference(const QString& description,
                                                        const QString& requestId)
{
    return VolcEngineRequest()
        .setPrompt(description)
        .setResolution(Resolution::Square_1x1)
        .setReturnUrl(true)
        .setRequestId(requestId);
}

VolcEngineRequest VolcEngineRequest::forPanel(const QString& description,
                                               const QString& requestId)
{
    return VolcEngineRequest()
        .setPrompt(description)
        .setResolution(Resolution::Standard_3x2)
        .setReturnUrl(true)
        .setRequestId(requestId);
}

void VolcEngineRequest::getResolutionSize(Resolution resolution, int& width, int& height)
{
    switch (resolution) {
        case Resolution::Square_1x1:
            width = 1328;
            height = 1328;
            break;
        case Resolution::Standard_4x3:
            width = 1472;
            height = 1104;
            break;
        case Resolution::Standard_3x2:
            width = 1584;
            height = 1056;
            break;
        case Resolution::Widescreen_16x9:
            width = 1664;
            height = 936;
            break;
        case Resolution::Ultrawide_21x9:
            width = 2016;
            height = 864;
            break;
        default:
            width = 1328;
            height = 1328;
            break;
    }
}
