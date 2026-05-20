#pragma once

#include <QString>

class AppInitializer
{
public:
    struct InitResult {
        bool databaseSuccess = false;
        bool qwenSuccess = false;
        bool qwenImageSuccess = false;
        bool volcEngineImageSuccess = false;
        bool storageSuccess = false;
        QString errorMessage;
    };
    
    static InitResult initialize();
    
private:
    AppInitializer() = delete;
    ~AppInitializer() = delete;
    
    static QString findConfigFile();
    static void logConfigSearchPaths();
    static bool initializeQwenClient(InitResult& result);
    static bool initializeQwenImageClient(InitResult& result);
    static bool initializeVolcEngineImageClient(InitResult& result);
    static bool initializeStorageClient(InitResult& result);
    static void ensureDefaultUserExists();
    static void registerTaskHandlers();
    static void connectAutoPortraitEdit();
};
