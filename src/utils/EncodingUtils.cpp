#include "EncodingUtils.h"
#include <QTextCodec>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

void EncodingUtils::initSystemEncoding()
{
#ifdef Q_OS_WIN
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
#endif
    
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
}

QString EncodingUtils::tryFixUtf8AsLatin1(const QString& str)
{
    QString fixed = QString::fromUtf8(str.toLatin1());
    return containsChinese(fixed) ? fixed : QString();
}

QString EncodingUtils::tryFixGbkAsUtf8(const QString& str)
{
    QTextCodec* gbkCodec = QTextCodec::codecForName("GBK");
    if (!gbkCodec) { return QString(); }
    
    QString fixed = gbkCodec->toUnicode(str.toUtf8());
    return containsChinese(fixed) ? fixed : QString();
}

QString EncodingUtils::fixGarbledString(const QString& garbledStr)
{
    if (garbledStr.isEmpty() || containsChinese(garbledStr)) {
        return garbledStr;
    }
    
    QString fixed = tryFixUtf8AsLatin1(garbledStr);
    if (!fixed.isEmpty()) { return fixed; }
    
    fixed = tryFixGbkAsUtf8(garbledStr);
    return fixed.isEmpty() ? garbledStr : fixed;
}

bool EncodingUtils::checkFileEncoding(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    QByteArray data = file.read(4096);
    file.close();
    
    return Enc::hasBom(data) || Enc::isValidUtf8(data);
}
