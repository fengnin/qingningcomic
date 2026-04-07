/****************************************************************************
** Meta object code from reading C++ file 'NovelViewModel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/viewmodels/NovelViewModel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NovelViewModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NovelViewModel_t {
    QByteArrayData data[17];
    char stringdata0[196];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NovelViewModel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NovelViewModel_t qt_meta_stringdata_NovelViewModel = {
    {
QT_MOC_LITERAL(0, 0, 14), // "NovelViewModel"
QT_MOC_LITERAL(1, 15, 12), // "novelsLoaded"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 12), // "QList<Novel>"
QT_MOC_LITERAL(4, 42, 6), // "novels"
QT_MOC_LITERAL(5, 49, 10), // "totalCount"
QT_MOC_LITERAL(6, 60, 11), // "novelLoaded"
QT_MOC_LITERAL(7, 72, 5), // "Novel"
QT_MOC_LITERAL(8, 78, 5), // "novel"
QT_MOC_LITERAL(9, 84, 12), // "novelCreated"
QT_MOC_LITERAL(10, 97, 12), // "novelUpdated"
QT_MOC_LITERAL(11, 110, 7), // "novelId"
QT_MOC_LITERAL(12, 118, 12), // "novelDeleted"
QT_MOC_LITERAL(13, 131, 19), // "currentNovelChanged"
QT_MOC_LITERAL(14, 151, 14), // "onNovelCreated"
QT_MOC_LITERAL(15, 166, 14), // "onNovelUpdated"
QT_MOC_LITERAL(16, 181, 14) // "onNovelDeleted"

    },
    "NovelViewModel\0novelsLoaded\0\0QList<Novel>\0"
    "novels\0totalCount\0novelLoaded\0Novel\0"
    "novel\0novelCreated\0novelUpdated\0novelId\0"
    "novelDeleted\0currentNovelChanged\0"
    "onNovelCreated\0onNovelUpdated\0"
    "onNovelDeleted"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NovelViewModel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   59,    2, 0x06 /* Public */,
       6,    1,   64,    2, 0x06 /* Public */,
       9,    1,   67,    2, 0x06 /* Public */,
      10,    1,   70,    2, 0x06 /* Public */,
      12,    1,   73,    2, 0x06 /* Public */,
      13,    1,   76,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      14,    1,   79,    2, 0x08 /* Private */,
      15,    1,   82,    2, 0x08 /* Private */,
      16,    1,   85,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    4,    5,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, 0x80000000 | 7,    8,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 7,    8,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,

       0        // eod
};

void NovelViewModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NovelViewModel *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->novelsLoaded((*reinterpret_cast< const QList<Novel>(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->novelLoaded((*reinterpret_cast< const Novel(*)>(_a[1]))); break;
        case 2: _t->novelCreated((*reinterpret_cast< const Novel(*)>(_a[1]))); break;
        case 3: _t->novelUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->novelDeleted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->currentNovelChanged((*reinterpret_cast< const Novel(*)>(_a[1]))); break;
        case 6: _t->onNovelCreated((*reinterpret_cast< const Novel(*)>(_a[1]))); break;
        case 7: _t->onNovelUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->onNovelDeleted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QList<Novel> >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Novel >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Novel >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Novel >(); break;
            }
            break;
        case 6:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Novel >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NovelViewModel::*)(const QList<Novel> & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelViewModel::novelsLoaded)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NovelViewModel::*)(const Novel & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelViewModel::novelLoaded)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NovelViewModel::*)(const Novel & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelViewModel::novelCreated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NovelViewModel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelViewModel::novelUpdated)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NovelViewModel::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelViewModel::novelDeleted)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (NovelViewModel::*)(const Novel & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelViewModel::currentNovelChanged)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NovelViewModel::staticMetaObject = { {
    QMetaObject::SuperData::link<BaseViewModel::staticMetaObject>(),
    qt_meta_stringdata_NovelViewModel.data,
    qt_meta_data_NovelViewModel,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NovelViewModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NovelViewModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NovelViewModel.stringdata0))
        return static_cast<void*>(this);
    return BaseViewModel::qt_metacast(_clname);
}

int NovelViewModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseViewModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void NovelViewModel::novelsLoaded(const QList<Novel> & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void NovelViewModel::novelLoaded(const Novel & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void NovelViewModel::novelCreated(const Novel & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void NovelViewModel::novelUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NovelViewModel::novelDeleted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void NovelViewModel::currentNovelChanged(const Novel & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
