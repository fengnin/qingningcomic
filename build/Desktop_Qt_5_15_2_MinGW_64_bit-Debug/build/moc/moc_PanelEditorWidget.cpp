/****************************************************************************
** Meta object code from reading C++ file 'PanelEditorWidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/components/PanelEditorWidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PanelEditorWidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_PanelEditorWidget_t {
    QByteArrayData data[19];
    char stringdata0[261];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_PanelEditorWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_PanelEditorWidget_t qt_meta_stringdata_PanelEditorWidget = {
    {
QT_MOC_LITERAL(0, 0, 17), // "PanelEditorWidget"
QT_MOC_LITERAL(1, 18, 23), // "sceneDescriptionChanged"
QT_MOC_LITERAL(2, 42, 0), // ""
QT_MOC_LITERAL(3, 43, 11), // "description"
QT_MOC_LITERAL(4, 55, 13), // "editSubmitted"
QT_MOC_LITERAL(5, 69, 8), // "editMode"
QT_MOC_LITERAL(6, 78, 11), // "instruction"
QT_MOC_LITERAL(7, 90, 8), // "maskPath"
QT_MOC_LITERAL(8, 99, 14), // "imageGenerated"
QT_MOC_LITERAL(9, 114, 8), // "imageUrl"
QT_MOC_LITERAL(10, 123, 6), // "closed"
QT_MOC_LITERAL(11, 130, 18), // "onSaveSceneClicked"
QT_MOC_LITERAL(12, 149, 17), // "onEditModeClicked"
QT_MOC_LITERAL(13, 167, 4), // "mode"
QT_MOC_LITERAL(14, 172, 19), // "onSubmitEditClicked"
QT_MOC_LITERAL(15, 192, 23), // "onSelectMaskFileClicked"
QT_MOC_LITERAL(16, 216, 23), // "onImageDownloadFinished"
QT_MOC_LITERAL(17, 240, 14), // "QNetworkReply*"
QT_MOC_LITERAL(18, 255, 5) // "reply"

    },
    "PanelEditorWidget\0sceneDescriptionChanged\0"
    "\0description\0editSubmitted\0editMode\0"
    "instruction\0maskPath\0imageGenerated\0"
    "imageUrl\0closed\0onSaveSceneClicked\0"
    "onEditModeClicked\0mode\0onSubmitEditClicked\0"
    "onSelectMaskFileClicked\0onImageDownloadFinished\0"
    "QNetworkReply*\0reply"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_PanelEditorWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    3,   62,    2, 0x06 /* Public */,
       8,    1,   69,    2, 0x06 /* Public */,
      10,    0,   72,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    0,   73,    2, 0x08 /* Private */,
      12,    1,   74,    2, 0x08 /* Private */,
      14,    0,   77,    2, 0x08 /* Private */,
      15,    0,   78,    2, 0x08 /* Private */,
      16,    1,   79,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString,    5,    6,    7,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   13,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 17,   18,

       0        // eod
};

void PanelEditorWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<PanelEditorWidget *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->sceneDescriptionChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->editSubmitted((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        case 2: _t->imageGenerated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->closed(); break;
        case 4: _t->onSaveSceneClicked(); break;
        case 5: _t->onEditModeClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->onSubmitEditClicked(); break;
        case 7: _t->onSelectMaskFileClicked(); break;
        case 8: _t->onImageDownloadFinished((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (PanelEditorWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelEditorWidget::sceneDescriptionChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (PanelEditorWidget::*)(int , const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelEditorWidget::editSubmitted)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (PanelEditorWidget::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelEditorWidget::imageGenerated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (PanelEditorWidget::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&PanelEditorWidget::closed)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject PanelEditorWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QFrame::staticMetaObject>(),
    qt_meta_stringdata_PanelEditorWidget.data,
    qt_meta_data_PanelEditorWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *PanelEditorWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PanelEditorWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_PanelEditorWidget.stringdata0))
        return static_cast<void*>(this);
    return QFrame::qt_metacast(_clname);
}

int PanelEditorWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void PanelEditorWidget::sceneDescriptionChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PanelEditorWidget::editSubmitted(int _t1, const QString & _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PanelEditorWidget::imageGenerated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PanelEditorWidget::closed()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
