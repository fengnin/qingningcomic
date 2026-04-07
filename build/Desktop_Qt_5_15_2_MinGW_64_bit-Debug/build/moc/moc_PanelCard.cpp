/****************************************************************************
** Meta object code from reading C++ file 'PanelCard.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/components/PanelCard.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PanelCard.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PanelCard_t {
    QByteArrayData data[22];
    char stringdata0[261];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PanelCard_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PanelCard_t qt_meta_stringdata_PanelCard = {
    {
QT_MOC_LITERAL(0, 0, 9), // "PanelCard"
QT_MOC_LITERAL(1, 10, 7), // "clicked"
QT_MOC_LITERAL(2, 18, 0), // ""
QT_MOC_LITERAL(3, 19, 11), // "panelNumber"
QT_MOC_LITERAL(4, 31, 11), // "dataChanged"
QT_MOC_LITERAL(5, 43, 11), // "description"
QT_MOC_LITERAL(6, 55, 14), // "imageGenerated"
QT_MOC_LITERAL(7, 70, 8), // "imageUrl"
QT_MOC_LITERAL(8, 79, 12), // "imageClicked"
QT_MOC_LITERAL(9, 92, 9), // "imagePath"
QT_MOC_LITERAL(10, 102, 16), // "onEditBtnClicked"
QT_MOC_LITERAL(11, 119, 25), // "onSceneDescriptionChanged"
QT_MOC_LITERAL(12, 145, 15), // "onEditSubmitted"
QT_MOC_LITERAL(13, 161, 8), // "editMode"
QT_MOC_LITERAL(14, 170, 11), // "instruction"
QT_MOC_LITERAL(15, 182, 8), // "maskPath"
QT_MOC_LITERAL(16, 191, 16), // "onImageGenerated"
QT_MOC_LITERAL(17, 208, 14), // "onEditorClosed"
QT_MOC_LITERAL(18, 223, 18), // "onAsyncImageLoaded"
QT_MOC_LITERAL(19, 242, 2), // "id"
QT_MOC_LITERAL(20, 245, 8), // "cacheKey"
QT_MOC_LITERAL(21, 254, 6) // "pixmap"

    },
    "PanelCard\0clicked\0\0panelNumber\0"
    "dataChanged\0description\0imageGenerated\0"
    "imageUrl\0imageClicked\0imagePath\0"
    "onEditBtnClicked\0onSceneDescriptionChanged\0"
    "onEditSubmitted\0editMode\0instruction\0"
    "maskPath\0onImageGenerated\0onEditorClosed\0"
    "onAsyncImageLoaded\0id\0cacheKey\0pixmap"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PanelCard[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   64,    2, 0x06 /* Public */,
       4,    2,   67,    2, 0x06 /* Public */,
       6,    2,   72,    2, 0x06 /* Public */,
       8,    1,   77,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      10,    0,   80,    2, 0x08 /* Private */,
      11,    1,   81,    2, 0x08 /* Private */,
      12,    3,   84,    2, 0x08 /* Private */,
      16,    1,   91,    2, 0x08 /* Private */,
      17,    0,   94,    2, 0x08 /* Private */,
      18,    3,   95,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,    7,
    QMetaType::Void, QMetaType::QString,    9,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString,   13,   14,   15,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QPixmap,   19,   20,   21,

       0        // eod
};

void PanelCard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PanelCard *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->clicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->dataChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->imageGenerated((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 3: _t->imageClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->onEditBtnClicked(); break;
        case 5: _t->onSceneDescriptionChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->onEditSubmitted((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 7: _t->onImageGenerated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->onEditorClosed(); break;
        case 9: _t->onAsyncImageLoaded((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QPixmap(*)>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PanelCard::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelCard::clicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PanelCard::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelCard::dataChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PanelCard::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelCard::imageGenerated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (PanelCard::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelCard::imageClicked)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject PanelCard::staticMetaObject = { {
    QMetaObject::SuperData::link<EditorCardBase::staticMetaObject>(),
    qt_meta_stringdata_PanelCard.data,
    qt_meta_data_PanelCard,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PanelCard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PanelCard::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PanelCard.stringdata0))
        return static_cast<void*>(this);
    return EditorCardBase::qt_metacast(_clname);
}

int PanelCard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = EditorCardBase::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void PanelCard::clicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PanelCard::dataChanged(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PanelCard::imageGenerated(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PanelCard::imageClicked(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
