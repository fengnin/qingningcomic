/****************************************************************************
** Meta object code from reading C++ file 'StoryboardViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/viewmodels/StoryboardViewModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StoryboardViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StoryboardViewModel_t {
    QByteArrayData data[34];
    char stringdata0[464];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StoryboardViewModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StoryboardViewModel_t qt_meta_stringdata_StoryboardViewModel = {
    {
QT_MOC_LITERAL(0, 0, 19), // "StoryboardViewModel"
QT_MOC_LITERAL(1, 20, 17), // "storyboardsLoaded"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 17), // "QList<Storyboard>"
QT_MOC_LITERAL(4, 57, 11), // "storyboards"
QT_MOC_LITERAL(5, 69, 16), // "storyboardLoaded"
QT_MOC_LITERAL(6, 86, 10), // "Storyboard"
QT_MOC_LITERAL(7, 97, 10), // "storyboard"
QT_MOC_LITERAL(8, 108, 12), // "panelsLoaded"
QT_MOC_LITERAL(9, 121, 12), // "QList<Panel>"
QT_MOC_LITERAL(10, 134, 6), // "panels"
QT_MOC_LITERAL(11, 141, 15), // "analysisStarted"
QT_MOC_LITERAL(12, 157, 7), // "novelId"
QT_MOC_LITERAL(13, 165, 16), // "analysisProgress"
QT_MOC_LITERAL(14, 182, 5), // "stage"
QT_MOC_LITERAL(15, 188, 8), // "progress"
QT_MOC_LITERAL(16, 197, 17), // "analysisCompleted"
QT_MOC_LITERAL(17, 215, 13), // "chapterNumber"
QT_MOC_LITERAL(18, 229, 14), // "analysisFailed"
QT_MOC_LITERAL(19, 244, 5), // "error"
QT_MOC_LITERAL(20, 250, 22), // "imageGenerationStarted"
QT_MOC_LITERAL(21, 273, 5), // "total"
QT_MOC_LITERAL(22, 279, 23), // "imageGenerationProgress"
QT_MOC_LITERAL(23, 303, 7), // "current"
QT_MOC_LITERAL(24, 311, 24), // "imageGenerationCompleted"
QT_MOC_LITERAL(25, 336, 7), // "success"
QT_MOC_LITERAL(26, 344, 6), // "failed"
QT_MOC_LITERAL(27, 351, 17), // "storyboardDeleted"
QT_MOC_LITERAL(28, 369, 17), // "onAnalysisStarted"
QT_MOC_LITERAL(29, 387, 19), // "onAnalysisCompleted"
QT_MOC_LITERAL(30, 407, 14), // "AnalysisResult"
QT_MOC_LITERAL(31, 422, 6), // "result"
QT_MOC_LITERAL(32, 429, 16), // "onAnalysisFailed"
QT_MOC_LITERAL(33, 446, 17) // "onStoryboardSaved"

    },
    "StoryboardViewModel\0storyboardsLoaded\0"
    "\0QList<Storyboard>\0storyboards\0"
    "storyboardLoaded\0Storyboard\0storyboard\0"
    "panelsLoaded\0QList<Panel>\0panels\0"
    "analysisStarted\0novelId\0analysisProgress\0"
    "stage\0progress\0analysisCompleted\0"
    "chapterNumber\0analysisFailed\0error\0"
    "imageGenerationStarted\0total\0"
    "imageGenerationProgress\0current\0"
    "imageGenerationCompleted\0success\0"
    "failed\0storyboardDeleted\0onAnalysisStarted\0"
    "onAnalysisCompleted\0AnalysisResult\0"
    "result\0onAnalysisFailed\0onStoryboardSaved"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StoryboardViewModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   89,    2, 0x06 /* Public */,
       5,    1,   92,    2, 0x06 /* Public */,
       8,    1,   95,    2, 0x06 /* Public */,
      11,    1,   98,    2, 0x06 /* Public */,
      13,    2,  101,    2, 0x06 /* Public */,
      16,    2,  106,    2, 0x06 /* Public */,
      18,    2,  111,    2, 0x06 /* Public */,
      20,    1,  116,    2, 0x06 /* Public */,
      22,    2,  119,    2, 0x06 /* Public */,
      24,    2,  124,    2, 0x06 /* Public */,
      27,    2,  129,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      28,    1,  134,    2, 0x08 /* Private */,
      29,    2,  137,    2, 0x08 /* Private */,
      32,    2,  142,    2, 0x08 /* Private */,
      33,    1,  147,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,
    QMetaType::Void, 0x80000000 | 9,   10,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   14,   15,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   12,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   12,   19,
    QMetaType::Void, QMetaType::Int,   21,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   23,   21,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,   25,   26,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   12,   17,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 30,   12,   31,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   12,   19,
    QMetaType::Void, QMetaType::QString,   12,

       0        // eod
};

void StoryboardViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StoryboardViewModel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->storyboardsLoaded((*reinterpret_cast< const QList<Storyboard>(*)>(_a[1]))); break;
        case 1: _t->storyboardLoaded((*reinterpret_cast< const Storyboard(*)>(_a[1]))); break;
        case 2: _t->panelsLoaded((*reinterpret_cast< const QList<Panel>(*)>(_a[1]))); break;
        case 3: _t->analysisStarted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->analysisProgress((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 5: _t->analysisCompleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->analysisFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 7: _t->imageGenerationStarted((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 8: _t->imageGenerationProgress((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 9: _t->imageGenerationCompleted((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 10: _t->storyboardDeleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 11: _t->onAnalysisStarted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 12: _t->onAnalysisCompleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const AnalysisResult(*)>(_a[2]))); break;
        case 13: _t->onAnalysisFailed((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 14: _t->onStoryboardSaved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<Storyboard> >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Storyboard >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<Panel> >(); break;
            }
            break;
        case 12:
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
            using _t = void (StoryboardViewModel::*)(const QList<Storyboard> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::storyboardsLoaded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const Storyboard & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::storyboardLoaded)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const QList<Panel> & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::panelsLoaded)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::analysisStarted)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::analysisProgress)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::analysisCompleted)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::analysisFailed)) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::imageGenerationStarted)) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::imageGenerationProgress)) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::imageGenerationCompleted)) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (StoryboardViewModel::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardViewModel::storyboardDeleted)) {
                *result = 10;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject StoryboardViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<BaseViewModel::staticMetaObject>(),
    qt_meta_stringdata_StoryboardViewModel.data,
    qt_meta_data_StoryboardViewModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StoryboardViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StoryboardViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StoryboardViewModel.stringdata0))
        return static_cast<void*>(this);
    return BaseViewModel::qt_metacast(_clname);
}

int StoryboardViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseViewModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void StoryboardViewModel::storyboardsLoaded(const QList<Storyboard> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StoryboardViewModel::storyboardLoaded(const Storyboard & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void StoryboardViewModel::panelsLoaded(const QList<Panel> & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void StoryboardViewModel::analysisStarted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void StoryboardViewModel::analysisProgress(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void StoryboardViewModel::analysisCompleted(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void StoryboardViewModel::analysisFailed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void StoryboardViewModel::imageGenerationStarted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void StoryboardViewModel::imageGenerationProgress(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void StoryboardViewModel::imageGenerationCompleted(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void StoryboardViewModel::storyboardDeleted(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
