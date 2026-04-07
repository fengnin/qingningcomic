/****************************************************************************
** Meta object code from reading C++ file 'StoryboardItem.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/components/StoryboardItem.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StoryboardItem.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_StoryboardItem_t {
    QByteArrayData data[16];
    char stringdata0[182];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_StoryboardItem_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_StoryboardItem_t qt_meta_stringdata_StoryboardItem = {
    {
QT_MOC_LITERAL(0, 0, 14), // "StoryboardItem"
QT_MOC_LITERAL(1, 15, 11), // "editClicked"
QT_MOC_LITERAL(2, 27, 0), // ""
QT_MOC_LITERAL(3, 28, 11), // "panelNumber"
QT_MOC_LITERAL(4, 40, 11), // "dataChanged"
QT_MOC_LITERAL(5, 52, 7), // "panelId"
QT_MOC_LITERAL(6, 60, 5), // "scene"
QT_MOC_LITERAL(7, 66, 8), // "shotType"
QT_MOC_LITERAL(8, 75, 11), // "cameraAngle"
QT_MOC_LITERAL(9, 87, 10), // "characters"
QT_MOC_LITERAL(10, 98, 8), // "dialogue"
QT_MOC_LITERAL(11, 107, 12), // "visualPrompt"
QT_MOC_LITERAL(12, 120, 14), // "visualPromptEn"
QT_MOC_LITERAL(13, 135, 16), // "onEditBtnClicked"
QT_MOC_LITERAL(14, 152, 13), // "onSaveClicked"
QT_MOC_LITERAL(15, 166, 15) // "onCancelClicked"

    },
    "StoryboardItem\0editClicked\0\0panelNumber\0"
    "dataChanged\0panelId\0scene\0shotType\0"
    "cameraAngle\0characters\0dialogue\0"
    "visualPrompt\0visualPromptEn\0"
    "onEditBtnClicked\0onSaveClicked\0"
    "onCancelClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_StoryboardItem[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   39,    2, 0x06 /* Public */,
       4,    9,   42,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      13,    0,   61,    2, 0x08 /* Private */,
      14,    0,   62,    2, 0x08 /* Private */,
      15,    0,   63,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString,    5,    3,    6,    7,    8,    9,   10,   11,   12,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void StoryboardItem::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<StoryboardItem *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->editClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->dataChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4])),(*reinterpret_cast< const QString(*)>(_a[5])),(*reinterpret_cast< const QString(*)>(_a[6])),(*reinterpret_cast< const QString(*)>(_a[7])),(*reinterpret_cast< const QString(*)>(_a[8])),(*reinterpret_cast< const QString(*)>(_a[9]))); break;
        case 2: _t->onEditBtnClicked(); break;
        case 3: _t->onSaveClicked(); break;
        case 4: _t->onCancelClicked(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (StoryboardItem::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardItem::editClicked)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (StoryboardItem::*)(const QString & , int , const QString & , const QString & , const QString & , const QString & , const QString & , const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&StoryboardItem::dataChanged)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject StoryboardItem::staticMetaObject = { {
    QMetaObject::SuperData::link<EditorCardBase::staticMetaObject>(),
    qt_meta_stringdata_StoryboardItem.data,
    qt_meta_data_StoryboardItem,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *StoryboardItem::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *StoryboardItem::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_StoryboardItem.stringdata0))
        return static_cast<void*>(this);
    return EditorCardBase::qt_metacast(_clname);
}

int StoryboardItem::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = EditorCardBase::qt_metacall(_c, _id, _a);
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
void StoryboardItem::editClicked(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void StoryboardItem::dataChanged(const QString & _t1, int _t2, const QString & _t3, const QString & _t4, const QString & _t5, const QString & _t6, const QString & _t7, const QString & _t8, const QString & _t9)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t5))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t6))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t7))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t8))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t9))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
