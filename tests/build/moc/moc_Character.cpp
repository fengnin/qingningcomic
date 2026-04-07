/****************************************************************************
** Meta object code from reading C++ file 'Character.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/Character.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Character.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Character_t {
    QByteArrayData data[9];
    char stringdata0[87];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Character_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Character_t qt_meta_stringdata_Character = {
    {
QT_MOC_LITERAL(0, 0, 9), // "Character"
QT_MOC_LITERAL(1, 10, 2), // "id"
QT_MOC_LITERAL(2, 13, 7), // "novelId"
QT_MOC_LITERAL(3, 21, 4), // "name"
QT_MOC_LITERAL(4, 26, 4), // "role"
QT_MOC_LITERAL(5, 31, 10), // "appearance"
QT_MOC_LITERAL(6, 42, 19), // "CharacterAppearance"
QT_MOC_LITERAL(7, 62, 11), // "personality"
QT_MOC_LITERAL(8, 74, 12) // "portraitPath"

    },
    "Character\0id\0novelId\0name\0role\0"
    "appearance\0CharacterAppearance\0"
    "personality\0portraitPath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Character[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       7,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, QMetaType::QString, 0x00095103,
       2, QMetaType::QString, 0x00095103,
       3, QMetaType::QString, 0x00095103,
       4, QMetaType::QString, 0x00095103,
       5, 0x80000000 | 6, 0x0009510b,
       7, QMetaType::QStringList, 0x00095103,
       8, QMetaType::QString, 0x00095103,

       0        // eod
};

void Character::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< CharacterAppearance >(); break;
        }
    }

#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = reinterpret_cast<Character *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->id(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->novelId(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->name(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->role(); break;
        case 4: *reinterpret_cast< CharacterAppearance*>(_v) = _t->appearance(); break;
        case 5: *reinterpret_cast< QStringList*>(_v) = _t->personality(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->portraitPath(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = reinterpret_cast<Character *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setId(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setNovelId(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setName(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setRole(*reinterpret_cast< QString*>(_v)); break;
        case 4: _t->setAppearance(*reinterpret_cast< CharacterAppearance*>(_v)); break;
        case 5: _t->setPersonality(*reinterpret_cast< QStringList*>(_v)); break;
        case 6: _t->setPortraitPath(*reinterpret_cast< QString*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
    Q_UNUSED(_o);
}

QT_INIT_METAOBJECT const QMetaObject Character::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_Character.data,
    qt_meta_data_Character,
    qt_static_metacall,
    nullptr,
    nullptr
} };

QT_WARNING_POP
QT_END_MOC_NAMESPACE
