/****************************************************************************
** Meta object code from reading C++ file 'SceneExtractor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/SceneExtractor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SceneExtractor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_SceneExtractor_t {
    QByteArrayData data[12];
    char stringdata0[138];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_SceneExtractor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_SceneExtractor_t qt_meta_stringdata_SceneExtractor = {
    {
QT_MOC_LITERAL(0, 0, 14), // "SceneExtractor"
QT_MOC_LITERAL(1, 15, 14), // "sceneExtracted"
QT_MOC_LITERAL(2, 30, 0), // ""
QT_MOC_LITERAL(3, 31, 14), // "ExtractedScene"
QT_MOC_LITERAL(4, 46, 5), // "scene"
QT_MOC_LITERAL(5, 52, 10), // "sceneSaved"
QT_MOC_LITERAL(6, 63, 7), // "sceneId"
QT_MOC_LITERAL(7, 71, 9), // "sceneName"
QT_MOC_LITERAL(8, 81, 12), // "sceneUpdated"
QT_MOC_LITERAL(9, 94, 17), // "sceneCountChanged"
QT_MOC_LITERAL(10, 112, 5), // "count"
QT_MOC_LITERAL(11, 118, 19) // "extractionCompleted"

    },
    "SceneExtractor\0sceneExtracted\0\0"
    "ExtractedScene\0scene\0sceneSaved\0sceneId\0"
    "sceneName\0sceneUpdated\0sceneCountChanged\0"
    "count\0extractionCompleted"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_SceneExtractor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,
       5,    2,   42,    2, 0x06 /* Public */,
       8,    2,   47,    2, 0x06 /* Public */,
       9,    1,   52,    2, 0x06 /* Public */,
      11,    1,   55,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    6,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    6,    7,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void, QMetaType::Int,   10,

       0        // eod
};

void SceneExtractor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SceneExtractor *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sceneExtracted((*reinterpret_cast< const ExtractedScene(*)>(_a[1]))); break;
        case 1: _t->sceneSaved((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->sceneUpdated((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->sceneCountChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 4: _t->extractionCompleted((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SceneExtractor::*)(const ExtractedScene & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneExtractor::sceneExtracted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SceneExtractor::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneExtractor::sceneSaved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SceneExtractor::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneExtractor::sceneUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (SceneExtractor::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneExtractor::sceneCountChanged)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (SceneExtractor::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&SceneExtractor::extractionCompleted)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject SceneExtractor::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_SceneExtractor.data,
    qt_meta_data_SceneExtractor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *SceneExtractor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SceneExtractor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_SceneExtractor.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SceneExtractor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SceneExtractor::sceneExtracted(const ExtractedScene & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SceneExtractor::sceneSaved(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SceneExtractor::sceneUpdated(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SceneExtractor::sceneCountChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void SceneExtractor::extractionCompleted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
