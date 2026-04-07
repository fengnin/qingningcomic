/****************************************************************************
** Meta object code from reading C++ file 'NovelService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/NovelService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NovelService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NovelService_t {
    QByteArrayData data[11];
    char stringdata0[106];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NovelService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NovelService_t qt_meta_stringdata_NovelService = {
    {
QT_MOC_LITERAL(0, 0, 12), // "NovelService"
QT_MOC_LITERAL(1, 13, 12), // "novelCreated"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 5), // "Novel"
QT_MOC_LITERAL(4, 33, 5), // "novel"
QT_MOC_LITERAL(5, 39, 12), // "novelUpdated"
QT_MOC_LITERAL(6, 52, 7), // "novelId"
QT_MOC_LITERAL(7, 60, 12), // "novelDeleted"
QT_MOC_LITERAL(8, 73, 13), // "statusChanged"
QT_MOC_LITERAL(9, 87, 11), // "NovelStatus"
QT_MOC_LITERAL(10, 99, 6) // "status"

    },
    "NovelService\0novelCreated\0\0Novel\0novel\0"
    "novelUpdated\0novelId\0novelDeleted\0"
    "statusChanged\0NovelStatus\0status"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NovelService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       5,    1,   37,    2, 0x06 /* Public */,
       7,    1,   40,    2, 0x06 /* Public */,
       8,    2,   43,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 9,    6,   10,

       0        // eod
};

void NovelService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NovelService *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->novelCreated((*reinterpret_cast< const Novel(*)>(_a[1]))); break;
        case 1: _t->novelUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->novelDeleted((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->statusChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< NovelStatus(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Novel >(); break;
            }
            break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< NovelStatus >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NovelService::*)(const Novel & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelService::novelCreated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NovelService::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelService::novelUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NovelService::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelService::novelDeleted)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NovelService::*)(const QString & , NovelStatus );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelService::statusChanged)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NovelService::staticMetaObject = { {
    QMetaObject::SuperData::link<BaseService::staticMetaObject>(),
    qt_meta_stringdata_NovelService.data,
    qt_meta_data_NovelService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NovelService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NovelService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NovelService.stringdata0))
        return static_cast<void*>(this);
    return BaseService::qt_metacast(_clname);
}

int NovelService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseService::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void NovelService::novelCreated(const Novel & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void NovelService::novelUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void NovelService::novelDeleted(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void NovelService::statusChanged(const QString & _t1, NovelStatus _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
