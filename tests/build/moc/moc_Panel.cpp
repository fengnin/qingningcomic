/****************************************************************************
** Meta object code from reading C++ file 'Panel.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/Panel.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Panel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Panel_t {
    QByteArrayData data[15];
    char stringdata0[133];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Panel_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Panel_t qt_meta_stringdata_Panel = {
    {
QT_MOC_LITERAL(0, 0, 5), // "Panel"
QT_MOC_LITERAL(1, 6, 2), // "id"
QT_MOC_LITERAL(2, 9, 12), // "storyboardId"
QT_MOC_LITERAL(3, 22, 4), // "page"
QT_MOC_LITERAL(4, 27, 5), // "index"
QT_MOC_LITERAL(5, 33, 5), // "scene"
QT_MOC_LITERAL(6, 39, 8), // "shotType"
QT_MOC_LITERAL(7, 48, 11), // "cameraAngle"
QT_MOC_LITERAL(8, 60, 12), // "visualPrompt"
QT_MOC_LITERAL(9, 73, 14), // "visualPromptEn"
QT_MOC_LITERAL(10, 88, 6), // "status"
QT_MOC_LITERAL(11, 95, 12), // "previewS3Key"
QT_MOC_LITERAL(12, 108, 7), // "hdS3Key"
QT_MOC_LITERAL(13, 116, 10), // "previewUrl"
QT_MOC_LITERAL(14, 127, 5) // "hdUrl"

    },
    "Panel\0id\0storyboardId\0page\0index\0scene\0"
    "shotType\0cameraAngle\0visualPrompt\0"
    "visualPromptEn\0status\0previewS3Key\0"
    "hdS3Key\0previewUrl\0hdUrl"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Panel[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
      14,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, QMetaType::QString, 0x00095103,
       2, QMetaType::QString, 0x00095103,
       3, QMetaType::Int, 0x00095103,
       4, QMetaType::Int, 0x00095103,
       5, QMetaType::QString, 0x00095103,
       6, QMetaType::QString, 0x00095103,
       7, QMetaType::QString, 0x00095103,
       8, QMetaType::QString, 0x00095103,
       9, QMetaType::QString, 0x00095103,
      10, QMetaType::QString, 0x00095103,
      11, QMetaType::QString, 0x00095103,
      12, QMetaType::QString, 0x00095103,
      13, QMetaType::QString, 0x00095103,
      14, QMetaType::QString, 0x00095103,

       0        // eod
};

void Panel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{

#ifndef QT_NO_PROPERTIES
    if (_c == QMetaObject::ReadProperty) {
        auto *_t = reinterpret_cast<Panel *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->id(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->storyboardId(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->page(); break;
        case 3: *reinterpret_cast< int*>(_v) = _t->index(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->scene(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->shotType(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->cameraAngle(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->visualPrompt(); break;
        case 8: *reinterpret_cast< QString*>(_v) = _t->visualPromptEn(); break;
        case 9: *reinterpret_cast< QString*>(_v) = _t->status(); break;
        case 10: *reinterpret_cast< QString*>(_v) = _t->previewS3Key(); break;
        case 11: *reinterpret_cast< QString*>(_v) = _t->hdS3Key(); break;
        case 12: *reinterpret_cast< QString*>(_v) = _t->previewUrl(); break;
        case 13: *reinterpret_cast< QString*>(_v) = _t->hdUrl(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = reinterpret_cast<Panel *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setId(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setStoryboardId(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setPage(*reinterpret_cast< int*>(_v)); break;
        case 3: _t->setIndex(*reinterpret_cast< int*>(_v)); break;
        case 4: _t->setScene(*reinterpret_cast< QString*>(_v)); break;
        case 5: _t->setShotType(*reinterpret_cast< QString*>(_v)); break;
        case 6: _t->setCameraAngle(*reinterpret_cast< QString*>(_v)); break;
        case 7: _t->setVisualPrompt(*reinterpret_cast< QString*>(_v)); break;
        case 8: _t->setVisualPromptEn(*reinterpret_cast< QString*>(_v)); break;
        case 9: _t->setStatus(*reinterpret_cast< QString*>(_v)); break;
        case 10: _t->setPreviewS3Key(*reinterpret_cast< QString*>(_v)); break;
        case 11: _t->setHdS3Key(*reinterpret_cast< QString*>(_v)); break;
        case 12: _t->setPreviewUrl(*reinterpret_cast< QString*>(_v)); break;
        case 13: _t->setHdUrl(*reinterpret_cast< QString*>(_v)); break;
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

QT_INIT_METAOBJECT const QMetaObject Panel::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_Panel.data,
    qt_meta_data_Panel,
    qt_static_metacall,
    nullptr,
    nullptr
} };

QT_WARNING_POP
QT_END_MOC_NAMESPACE
