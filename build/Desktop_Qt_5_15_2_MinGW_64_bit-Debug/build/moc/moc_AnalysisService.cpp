/****************************************************************************
** Meta object code from reading C++ file 'AnalysisService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/AnalysisService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'AnalysisService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_AnalysisService_t {
    QByteArrayData data[33];
    char stringdata0[438];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_AnalysisService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_AnalysisService_t qt_meta_stringdata_AnalysisService = {
    {
QT_MOC_LITERAL(0, 0, 15), // "AnalysisService"
QT_MOC_LITERAL(1, 16, 15), // "analysisStarted"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 7), // "novelId"
QT_MOC_LITERAL(4, 41, 16), // "analysisProgress"
QT_MOC_LITERAL(5, 58, 7), // "current"
QT_MOC_LITERAL(6, 66, 5), // "total"
QT_MOC_LITERAL(7, 72, 7), // "message"
QT_MOC_LITERAL(8, 80, 17), // "analysisCompleted"
QT_MOC_LITERAL(9, 98, 14), // "AnalysisResult"
QT_MOC_LITERAL(10, 113, 6), // "result"
QT_MOC_LITERAL(11, 120, 14), // "analysisFailed"
QT_MOC_LITERAL(12, 135, 5), // "error"
QT_MOC_LITERAL(13, 141, 14), // "streamProgress"
QT_MOC_LITERAL(14, 156, 14), // "partialContent"
QT_MOC_LITERAL(15, 171, 15), // "streamCompleted"
QT_MOC_LITERAL(16, 187, 23), // "imageGenerationProgress"
QT_MOC_LITERAL(17, 211, 14), // "onQwenProgress"
QT_MOC_LITERAL(18, 226, 11), // "onQwenError"
QT_MOC_LITERAL(19, 238, 9), // "operation"
QT_MOC_LITERAL(20, 248, 20), // "onQwenStreamProgress"
QT_MOC_LITERAL(21, 269, 21), // "onQwenStreamCompleted"
QT_MOC_LITERAL(22, 291, 13), // "onTaskStarted"
QT_MOC_LITERAL(23, 305, 6), // "taskId"
QT_MOC_LITERAL(24, 312, 14), // "onTaskProgress"
QT_MOC_LITERAL(25, 327, 8), // "progress"
QT_MOC_LITERAL(26, 336, 15), // "onTaskCompleted"
QT_MOC_LITERAL(27, 352, 12), // "onTaskFailed"
QT_MOC_LITERAL(28, 365, 20), // "onBibleImageProgress"
QT_MOC_LITERAL(29, 386, 4), // "info"
QT_MOC_LITERAL(30, 391, 21), // "onBibleImageCompleted"
QT_MOC_LITERAL(31, 413, 12), // "successCount"
QT_MOC_LITERAL(32, 426, 11) // "failedCount"

    },
    "AnalysisService\0analysisStarted\0\0"
    "novelId\0analysisProgress\0current\0total\0"
    "message\0analysisCompleted\0AnalysisResult\0"
    "result\0analysisFailed\0error\0streamProgress\0"
    "partialContent\0streamCompleted\0"
    "imageGenerationProgress\0onQwenProgress\0"
    "onQwenError\0operation\0onQwenStreamProgress\0"
    "onQwenStreamCompleted\0onTaskStarted\0"
    "taskId\0onTaskProgress\0progress\0"
    "onTaskCompleted\0onTaskFailed\0"
    "onBibleImageProgress\0info\0"
    "onBibleImageCompleted\0successCount\0"
    "failedCount"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_AnalysisService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      17,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   99,    2, 0x06 /* Public */,
       4,    3,  102,    2, 0x06 /* Public */,
       8,    2,  109,    2, 0x06 /* Public */,
      11,    2,  114,    2, 0x06 /* Public */,
      13,    1,  119,    2, 0x06 /* Public */,
      15,    2,  122,    2, 0x06 /* Public */,
      16,    3,  127,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      17,    3,  134,    2, 0x08 /* Private */,
      18,    2,  141,    2, 0x08 /* Private */,
      20,    1,  146,    2, 0x08 /* Private */,
      21,    1,  149,    2, 0x08 /* Private */,
      22,    1,  152,    2, 0x08 /* Private */,
      24,    3,  155,    2, 0x08 /* Private */,
      26,    2,  162,    2, 0x08 /* Private */,
      27,    2,  167,    2, 0x08 /* Private */,
      28,    3,  172,    2, 0x08 /* Private */,
      30,    2,  179,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    5,    6,    7,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 9,    3,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,   12,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonObject,    3,   10,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    5,    6,    7,

 // slots: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    5,    6,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   19,    7,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void, QMetaType::QJsonObject,   10,
    QMetaType::Void, QMetaType::QString,   23,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::QString,   23,   25,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonObject,   23,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   23,   12,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,    5,    6,   29,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   31,   32,

       0        // eod
};

void AnalysisService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<AnalysisService *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->analysisStarted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->analysisProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 2: _t->analysisCompleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const AnalysisResult(*)>(_a[2]))); break;
        case 3: _t->analysisFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->streamProgress((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->streamCompleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 6: _t->imageGenerationProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 7: _t->onQwenProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 8: _t->onQwenError((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 9: _t->onQwenStreamProgress((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->onQwenStreamCompleted((*reinterpret_cast< const QJsonObject(*)>(_a[1]))); break;
        case 11: _t->onTaskStarted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->onTaskProgress((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 13: _t->onTaskCompleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 14: _t->onTaskFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 15: _t->onBibleImageProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 16: _t->onBibleImageCompleted((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< AnalysisResult >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (AnalysisService::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::analysisStarted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (AnalysisService::*)(int , int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::analysisProgress)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (AnalysisService::*)(const QString & , const AnalysisResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::analysisCompleted)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (AnalysisService::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::analysisFailed)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (AnalysisService::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::streamProgress)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (AnalysisService::*)(const QString & , const QJsonObject & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::streamCompleted)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (AnalysisService::*)(int , int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&AnalysisService::imageGenerationProgress)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject AnalysisService::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_AnalysisService.data,
    qt_meta_data_AnalysisService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *AnalysisService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AnalysisService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_AnalysisService.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AnalysisService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 17)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 17;
    }
    return _id;
}

// SIGNAL 0
void AnalysisService::analysisStarted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void AnalysisService::analysisProgress(int _t1, int _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void AnalysisService::analysisCompleted(const QString & _t1, const AnalysisResult & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void AnalysisService::analysisFailed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void AnalysisService::streamProgress(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void AnalysisService::streamCompleted(const QString & _t1, const QJsonObject & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void AnalysisService::imageGenerationProgress(int _t1, int _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
