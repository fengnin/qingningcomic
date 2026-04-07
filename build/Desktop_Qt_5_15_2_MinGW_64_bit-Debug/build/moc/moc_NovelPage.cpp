/****************************************************************************
** Meta object code from reading C++ file 'NovelPage.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/NovelPage.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NovelPage.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_NovelPage_t {
    QByteArrayData data[13];
    char stringdata0[192];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_NovelPage_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_NovelPage_t qt_meta_stringdata_NovelPage = {
    {
QT_MOC_LITERAL(0, 0, 9), // "NovelPage"
QT_MOC_LITERAL(1, 10, 15), // "showNovelDetail"
QT_MOC_LITERAL(2, 26, 0), // ""
QT_MOC_LITERAL(3, 27, 7), // "novelId"
QT_MOC_LITERAL(4, 35, 20), // "createNovelRequested"
QT_MOC_LITERAL(5, 56, 18), // "editNovelRequested"
QT_MOC_LITERAL(6, 75, 20), // "onCreateNovelClicked"
QT_MOC_LITERAL(7, 96, 13), // "onJumpClicked"
QT_MOC_LITERAL(8, 110, 15), // "onFilterClicked"
QT_MOC_LITERAL(9, 126, 6), // "status"
QT_MOC_LITERAL(10, 133, 18), // "onNovelCardClicked"
QT_MOC_LITERAL(11, 152, 20), // "onDeleteNovelClicked"
QT_MOC_LITERAL(12, 173, 18) // "onEditNovelClicked"

    },
    "NovelPage\0showNovelDetail\0\0novelId\0"
    "createNovelRequested\0editNovelRequested\0"
    "onCreateNovelClicked\0onJumpClicked\0"
    "onFilterClicked\0status\0onNovelCardClicked\0"
    "onDeleteNovelClicked\0onEditNovelClicked"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_NovelPage[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    0,   62,    2, 0x06 /* Public */,
       5,    1,   63,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   66,    2, 0x08 /* Private */,
       7,    0,   67,    2, 0x08 /* Private */,
       8,    1,   68,    2, 0x08 /* Private */,
      10,    1,   71,    2, 0x08 /* Private */,
      11,    1,   74,    2, 0x08 /* Private */,
      12,    1,   77,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    3,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::QString,    3,

       0        // eod
};

void NovelPage::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NovelPage *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->showNovelDetail((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->createNovelRequested(); break;
        case 2: _t->editNovelRequested((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->onCreateNovelClicked(); break;
        case 4: _t->onJumpClicked(); break;
        case 5: _t->onFilterClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->onNovelCardClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->onDeleteNovelClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->onEditNovelClicked((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NovelPage::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelPage::showNovelDetail)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NovelPage::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelPage::createNovelRequested)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NovelPage::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&NovelPage::editNovelRequested)) {
                *result = 2;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject NovelPage::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_NovelPage.data,
    qt_meta_data_NovelPage,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *NovelPage::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NovelPage::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_NovelPage.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int NovelPage::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void NovelPage::showNovelDetail(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void NovelPage::createNovelRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NovelPage::editNovelRequested(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
