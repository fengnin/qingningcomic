# 数据库事务测试项目配置
TARGET = test_database_transaction
TEMPLATE = app

CONFIG += c++11 console
CONFIG += testcase

QT += testlib sql concurrent network

# 头文件
HEADERS += \
    $$PWD/../include/DatabaseManager.h \
    $$PWD/../include/Logger.h \
    $$PWD/../include/utils/SingletonUtils.h

# 源文件
SOURCES += \
    $$PWD/../src/data/DatabaseManager.cpp \
    $$PWD/../src/services/ServiceContainer.cpp \
    $$PWD/../src/utils/Logger.cpp \
    test_database_transaction.cpp

# 包含目录
INCLUDEPATH += \
    $$PWD/../include \
    $$PWD/../src

# 输出目录
DESTDIR = $$PWD/bin
OBJECTS_DIR = $$PWD/build/obj
MOC_DIR = $$PWD/build/moc

# Windows 编码设置
win32-g++ {
    QMAKE_CXXFLAGS += -fexec-charset=UTF-8
    QMAKE_CXXFLAGS += -finput-charset=UTF-8
}
