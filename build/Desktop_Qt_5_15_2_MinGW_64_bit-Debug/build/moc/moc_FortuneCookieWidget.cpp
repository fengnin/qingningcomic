/****************************************************************************
** Meta object code from reading C++ file 'FortuneCookieWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/FortuneCookieWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'FortuneCookieWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_FortuneCookieWidget_t {
    QByteArrayData data[11];
    char stringdata0[130];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_FortuneCookieWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_FortuneCookieWidget_t qt_meta_stringdata_FortuneCookieWidget = {
    {
QT_MOC_LITERAL(0, 0, 19), // "FortuneCookieWidget"
QT_MOC_LITERAL(1, 20, 15), // "fortuneRevealed"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 11), // "FortuneData"
QT_MOC_LITERAL(4, 49, 7), // "fortune"
QT_MOC_LITERAL(5, 57, 12), // "stateChanged"
QT_MOC_LITERAL(6, 70, 11), // "CookieState"
QT_MOC_LITERAL(7, 82, 8), // "newState"
QT_MOC_LITERAL(8, 91, 14), // "cookieRotation"
QT_MOC_LITERAL(9, 106, 11), // "cookieScale"
QT_MOC_LITERAL(10, 118, 11) // "glowOpacity"

    },
    "FortuneCookieWidget\0fortuneRevealed\0"
    "\0FortuneData\0fortune\0stateChanged\0"
    "CookieState\0newState\0cookieRotation\0"
    "cookieScale\0glowOpacity"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_FortuneCookieWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       3,   30, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   24,    2, 0x06 /* Public */,
       5,    1,   27,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    7,

 // properties: name, type, flags
       8, QMetaType::QReal, 0x00095103,
       9, QMetaType::QReal, 0x00095103,
      10, QMetaType::QReal, 0x00095103,

       0        // eod
};

void FortuneCookieWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<FortuneCookieWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->fortuneRevealed((*reinterpret_cast< const FortuneData(*)>(_a[1]))); break;
        case 1: _t->stateChanged((*reinterpret_cast< CookieState(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (FortuneCookieWidget::*)(const FortuneData & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FortuneCookieWidget::fortuneRevealed)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (FortuneCookieWidget::*)(CookieState );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&FortuneCookieWidget::stateChanged)) {
                *result = 1;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<FortuneCookieWidget *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< qreal*>(_v) = _t->cookieRotation(); break;
        case 1: *reinterpret_cast< qreal*>(_v) = _t->cookieScale(); break;
        case 2: *reinterpret_cast< qreal*>(_v) = _t->glowOpacity(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<FortuneCookieWidget *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setCookieRotation(*reinterpret_cast< qreal*>(_v)); break;
        case 1: _t->setCookieScale(*reinterpret_cast< qreal*>(_v)); break;
        case 2: _t->setGlowOpacity(*reinterpret_cast< qreal*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
}

QT_INIT_METAOBJECT const QMetaObject FortuneCookieWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_FortuneCookieWidget.data,
    qt_meta_data_FortuneCookieWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *FortuneCookieWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *FortuneCookieWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_FortuneCookieWidget.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int FortuneCookieWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyDesignable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyScriptable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyStored) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyEditable) {
        _id -= 3;
    } else if (_c == QMetaObject::QueryPropertyUser) {
        _id -= 3;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void FortuneCookieWidget::fortuneRevealed(const FortuneData & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void FortuneCookieWidget::stateChanged(CookieState _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
