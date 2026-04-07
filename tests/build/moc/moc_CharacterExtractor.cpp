/****************************************************************************
** Meta object code from reading C++ file 'CharacterExtractor.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/CharacterExtractor.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CharacterExtractor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CharacterExtractor_t {
    QByteArrayData data[9];
    char stringdata0[121];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CharacterExtractor_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CharacterExtractor_t qt_meta_stringdata_CharacterExtractor = {
    {
QT_MOC_LITERAL(0, 0, 18), // "CharacterExtractor"
QT_MOC_LITERAL(1, 19, 18), // "characterExtracted"
QT_MOC_LITERAL(2, 38, 0), // ""
QT_MOC_LITERAL(3, 39, 18), // "ExtractedCharacter"
QT_MOC_LITERAL(4, 58, 9), // "character"
QT_MOC_LITERAL(5, 68, 14), // "characterSaved"
QT_MOC_LITERAL(6, 83, 11), // "characterId"
QT_MOC_LITERAL(7, 95, 19), // "extractionCompleted"
QT_MOC_LITERAL(8, 115, 5) // "count"

    },
    "CharacterExtractor\0characterExtracted\0"
    "\0ExtractedCharacter\0character\0"
    "characterSaved\0characterId\0"
    "extractionCompleted\0count"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CharacterExtractor[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,
       5,    1,   32,    2, 0x06 /* Public */,
       7,    1,   35,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Int,    8,

       0        // eod
};

void CharacterExtractor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CharacterExtractor *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->characterExtracted((*reinterpret_cast< const ExtractedCharacter(*)>(_a[1]))); break;
        case 1: _t->characterSaved((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->extractionCompleted((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CharacterExtractor::*)(const ExtractedCharacter & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CharacterExtractor::characterExtracted)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CharacterExtractor::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CharacterExtractor::characterSaved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CharacterExtractor::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CharacterExtractor::extractionCompleted)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CharacterExtractor::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CharacterExtractor.data,
    qt_meta_data_CharacterExtractor,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CharacterExtractor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CharacterExtractor::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CharacterExtractor.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int CharacterExtractor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void CharacterExtractor::characterExtracted(const ExtractedCharacter & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CharacterExtractor::characterSaved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CharacterExtractor::extractionCompleted(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
