/****************************************************************************
** Meta object code from reading C++ file 'BibleItem.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/components/BibleItem.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BibleItem.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BibleItem_t {
    QByteArrayData data[19];
    char stringdata0[204];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BibleItem_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BibleItem_t qt_meta_stringdata_BibleItem = {
    {
QT_MOC_LITERAL(0, 0, 9), // "BibleItem"
QT_MOC_LITERAL(1, 10, 11), // "editClicked"
QT_MOC_LITERAL(2, 22, 0), // ""
QT_MOC_LITERAL(3, 23, 4), // "name"
QT_MOC_LITERAL(4, 28, 11), // "dataChanged"
QT_MOC_LITERAL(5, 40, 7), // "details"
QT_MOC_LITERAL(6, 48, 12), // "imageClicked"
QT_MOC_LITERAL(7, 61, 9), // "imagePath"
QT_MOC_LITERAL(8, 71, 13), // "uploadClicked"
QT_MOC_LITERAL(9, 85, 9), // "BibleType"
QT_MOC_LITERAL(10, 95, 4), // "type"
QT_MOC_LITERAL(11, 100, 18), // "deleteImageClicked"
QT_MOC_LITERAL(12, 119, 16), // "onEditBtnClicked"
QT_MOC_LITERAL(13, 136, 13), // "onSaveClicked"
QT_MOC_LITERAL(14, 150, 15), // "onCancelClicked"
QT_MOC_LITERAL(15, 166, 18), // "onAsyncImageLoaded"
QT_MOC_LITERAL(16, 185, 2), // "id"
QT_MOC_LITERAL(17, 188, 8), // "cacheKey"
QT_MOC_LITERAL(18, 197, 6) // "pixmap"

    },
    "BibleItem\0editClicked\0\0name\0dataChanged\0"
    "details\0imageClicked\0imagePath\0"
    "uploadClicked\0BibleType\0type\0"
    "deleteImageClicked\0onEditBtnClicked\0"
    "onSaveClicked\0onCancelClicked\0"
    "onAsyncImageLoaded\0id\0cacheKey\0pixmap"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BibleItem[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    2,   62,    2, 0x06 /* Public */,
       6,    1,   67,    2, 0x06 /* Public */,
       8,    2,   70,    2, 0x06 /* Public */,
      11,    2,   75,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    0,   80,    2, 0x08 /* Private */,
      13,    0,   81,    2, 0x08 /* Private */,
      14,    0,   82,    2, 0x08 /* Private */,
      15,    3,   83,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::QStringList,    3,    5,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 9,    3,   10,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 9,    3,   10,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QPixmap,   16,   17,   18,

       0        // eod
};

void BibleItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BibleItem *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->editClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->dataChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QStringList(*)>(_a[2]))); break;
        case 2: _t->imageClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->uploadClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< BibleType(*)>(_a[2]))); break;
        case 4: _t->deleteImageClicked((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< BibleType(*)>(_a[2]))); break;
        case 5: _t->onEditBtnClicked(); break;
        case 6: _t->onSaveClicked(); break;
        case 7: _t->onCancelClicked(); break;
        case 8: _t->onAsyncImageLoaded((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QPixmap(*)>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 3:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< BibleType >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 1:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< BibleType >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BibleItem::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleItem::editClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BibleItem::*)(const QString & , const QStringList & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleItem::dataChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BibleItem::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleItem::imageClicked)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BibleItem::*)(const QString & , BibleType );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleItem::uploadClicked)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (BibleItem::*)(const QString & , BibleType );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleItem::deleteImageClicked)) {
                *result = 4;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BibleItem::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_BibleItem.data,
    qt_meta_data_BibleItem,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BibleItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BibleItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BibleItem.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int BibleItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
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
void BibleItem::editClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BibleItem::dataChanged(const QString & _t1, const QStringList & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void BibleItem::imageClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void BibleItem::uploadClicked(const QString & _t1, BibleType _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void BibleItem::deleteImageClicked(const QString & _t1, BibleType _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
