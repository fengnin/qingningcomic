#ifndef ENCODINGUTILS_H
#define ENCODINGUTILS_H

#include <QString>
#include <QByteArray>
#include <QFile>
#include <QTextStream>
#include <utility>

namespace Enc {
    inline QString utf8(const char* str) 
    { 
        return QString::fromUtf8(str); 
    }
    
    template<typename... Args>
    inline QString fmt(const char* str, Args&&... args) 
    {
        return QString::fromUtf8(str).arg(std::forward<Args>(args)...);
    }
    
    inline QString json(const QByteArray& data)
    {
        return QString::fromUtf8(data);
    }
    
    inline bool containsChinese(const QString& str) 
    {
        for (const QChar& c : str) {
            ushort u = c.unicode();
            if (u >= 0x4E00 && u <= 0x9FFF) {
                return true;
            }
        }
        return false;
    }
    
    inline bool isValidUtf8(const QByteArray& data) 
    {
        const uchar* p = reinterpret_cast<const uchar*>(data.constData());
        const uchar* end = p + data.size();
        
        while (p < end) {
            uchar c = *p;
            int len;
            
            if (c < 0x80) { len = 1; }
            else if ((c & 0xE0) == 0xC0) { len = 2; }
            else if ((c & 0xF0) == 0xE0) { len = 3; }
            else if ((c & 0xF8) == 0xF0) { len = 4; }
            else { return false; }
            
            if (p + len > end) { return false; }
            
            for (int i = 1; i < len; ++i) {
                if ((p[i] & 0xC0) != 0x80) { return false; }
            }
            p += len;
        }
        return true;
    }
    
    inline bool hasBom(const QByteArray& data) 
    {
        return data.size() >= 3 && 
               (uchar)data[0] == 0xEF &&
               (uchar)data[1] == 0xBB &&
               (uchar)data[2] == 0xBF;
    }
    
    inline QByteArray removeBom(const QByteArray& data) 
    {
        return hasBom(data) ? data.mid(3) : data;
    }
    
    inline QString readFile(const QString& path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString content = in.readAll();
        file.close();
        return content;
    }
    
    inline bool writeFile(const QString& path, const QString& content)
    {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            return false;
        }
        QTextStream out(&file);
        out.setCodec("UTF-8");
        out.setGenerateByteOrderMark(false);
        out << content;
        file.close();
        return true;
    }
}

#define TR(str) Enc::utf8(str)
#define TR_FMT(...) Enc::fmt(__VA_ARGS__)

class EncodingUtils
{
public:
    static void initSystemEncoding();
    static QString fixGarbledString(const QString& garbledStr);
    static bool checkFileEncoding(const QString& filePath);
    
    static bool containsChinese(const QString& str) { return Enc::containsChinese(str); }
    static bool isValidUtf8(const QByteArray& data) { return Enc::isValidUtf8(data); }
    static QString readFile(const QString& path) { return Enc::readFile(path); }
    static bool writeFile(const QString& path, const QString& content) { return Enc::writeFile(path, content); }

private:
    EncodingUtils() = delete;
    
    static QString tryFixUtf8AsLatin1(const QString& str);
    static QString tryFixGbkAsUtf8(const QString& str);
};

#endif // ENCODINGUTILS_H
