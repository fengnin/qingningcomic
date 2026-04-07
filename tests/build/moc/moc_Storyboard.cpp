/****************************************************************************
** Meta object code from reading C++ file 'Storyboard.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/Storyboard.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Storyboard.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Storyboard_t {
    QByteArrayData data[7];
    char stringdata0[66];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Storyboard_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Storyboard_t qt_meta_stringdata_Storyboard = {
    {
QT_MOC_LITERAL(0, 0, 10), // "Storyboard"
QT_MOC_LITERAL(1, 11, 2), // "id"
QT_MOC_LITERAL(2, 14, 7), // "novelId"
QT_MOC_LITERAL(3, 22, 13), // "chapterNumber"
QT_MOC_LITERAL(4, 36, 10), // "totalPages"
QT_MOC_LITERAL(5, 47, 10), // "panelCount"
QT_MOC_LITERAL(6, 58, 7) // "version"

    },
    "Storyboard\0id\0novelId\0chapterNumber\0"
    "totalPages\0panelCount\0version"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Storyboard[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       6,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, QMetaType::QString, 0x00095103,
       2, QMetaType::QString, 0x00095103,
       3, QMetaType::Int, 0x00095103,
       4, QMetaType::Int, 0x00095103,
       5, QMetaType::Int, 0x00095103,
       6, QMetaType::Int, 0x00095103,

       0        // eod
};

void Storyboard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{

#ifndef QT_NO_PROPERTIES
    if (_c == QMetaObject::ReadProperty) {
        auto *_t = reinterpret_cast<Storyboard *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->id(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->novelId(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->chapterNumber(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->totalPages(); break;
        case 4: *reinterpret_cast< int*>(_v) = _t->panelCount(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->version(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = reinterpret_cast<Storyboard *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setId(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setNovelId(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setChapterNumber(*reinterpret_cast< int*>(_v)); break;
        case 3: _t->setTotalPages(*reinterpret_cast< int*>(_v)); break;
        case 4: _t->setPanelCount(*reinterpret_cast< int*>(_v)); break;
        case 5: _t->setVersion(*reinterpret_cast< int*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject Storyboard::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_Storyboard.data,
    qt_meta_data_Storyboard,
    qt_static_metacall,
    nullptr,
    nullptr
} };

QT_WARNING_POP
QT_END_MOC_NAMESPACE
