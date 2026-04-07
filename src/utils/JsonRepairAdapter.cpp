#include "JsonRepairAdapter.h"
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QDebug>
#include <Logger.h>

namespace JsonRepair {

static QString getPythonScriptPath()
{
    QDir appDir(QCoreApplication::applicationDirPath());
    
    // 尝试多个可能的路径
    QStringList possiblePaths = {
        appDir.absoluteFilePath("scripts/repair_json_script.py"),
        appDir.absoluteFilePath("../scripts/repair_json_script.py"),
        appDir.absoluteFilePath("../../comic/scripts/repair_json_script.py"),
        appDir.absoluteFilePath("../../../comic/scripts/repair_json_script.py"),
    };
    
    // 也尝试从源代码目录查找
    QDir projectRoot(appDir);
    while (projectRoot.cdUp()) {
        QString scriptPath = projectRoot.absoluteFilePath("comic/scripts/repair_json_script.py");
        if (QFile::exists(scriptPath)) {
            return scriptPath;
        }
        if (projectRoot.isRoot()) break;
    }
    
    for (const QString& path : possiblePaths) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    
    // 返回默认路径（即使不存在，也会在后面报错）
    return appDir.absoluteFilePath("scripts/repair_json_script.py");
}

static QString findPythonExecutable()
{
    QStringList pythonCommands = {"python", "python3", "python.exe", "python3.exe"};
    
    for (const QString& cmd : pythonCommands) {
        QProcess process;
        process.start(cmd, QStringList() << "--version");
        if (process.waitForFinished(2000) && process.exitCode() == 0) {
            return cmd;
        }
    }
    
    return QString();
}

RepairResult repair(const QString& json)
{
    RepairResult result;
    
    if (json.isEmpty()) {
        result.error = "Empty input";
        return result;
    }
    
    QString pythonExe = findPythonExecutable();
    if (pythonExe.isEmpty()) {
        result.error = "Python not found. Please install Python and add it to PATH.";
        LOG_ERROR("JsonRepair", result.error);
        return result;
    }
    
    QString scriptPath = getPythonScriptPath();
    if (!QFile::exists(scriptPath)) {
        result.error = QString("JSON repair script not found: %1").arg(scriptPath);
        LOG_ERROR("JsonRepair", result.error);
        return result;
    }
    
    QProcess process;
    process.start(pythonExe, QStringList() << scriptPath << json);
    
    if (!process.waitForFinished(30000)) {
        result.error = "JSON repair timeout";
        LOG_ERROR("JsonRepair", result.error);
        return result;
    }
    
    QString output = QString::fromUtf8(process.readAllStandardOutput());
    QString errorOutput = QString::fromUtf8(process.readAllStandardError());
    
    if (process.exitCode() != 0) {
        result.error = QString("JSON repair failed: %1").arg(errorOutput);
        LOG_ERROR("JsonRepair", result.error);
        return result;
    }
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        result.error = QString("JSON parse error after repair: %1 at offset %2")
            .arg(parseError.errorString())
            .arg(parseError.offset);
        LOG_ERROR("JsonRepair", result.error);
        return result;
    }
    
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("error")) {
            result.error = obj["error"].toString();
            LOG_ERROR("JsonRepair", result.error);
            return result;
        }
        result.value = QJsonValue(obj);
    } else if (doc.isArray()) {
        result.value = QJsonValue(doc.array());
    } else {
        result.error = "Invalid JSON structure";
        LOG_ERROR("JsonRepair", result.error);
        return result;
    }
    
    result.success = true;
    LOG_DEBUG("JsonRepair", "JSON repair successful");
    
    return result;
}

QJsonObject repairToObject(const QString& json)
{
    RepairResult result = repair(json);
    if (result.success && result.value.isObject()) {
        return result.value.toObject();
    }
    return QJsonObject();
}

QJsonArray repairToArray(const QString& json)
{
    RepairResult result = repair(json);
    if (result.success && result.value.isArray()) {
        return result.value.toArray();
    }
    return QJsonArray();
}

}
