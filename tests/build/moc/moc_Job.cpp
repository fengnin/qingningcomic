/****************************************************************************
** Meta object code from reading C++ file 'Job.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../include/Job.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Job.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_Job_t {
    QByteArrayData data[12];
    char stringdata0[93];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_Job_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_Job_t qt_meta_stringdata_Job = {
    {
QT_MOC_LITERAL(0, 0, 3), // "Job"
QT_MOC_LITERAL(1, 4, 2), // "id"
QT_MOC_LITERAL(2, 7, 4), // "type"
QT_MOC_LITERAL(3, 12, 6), // "status"
QT_MOC_LITERAL(4, 19, 9), // "JobStatus"
QT_MOC_LITERAL(5, 29, 7), // "novelId"
QT_MOC_LITERAL(6, 37, 12), // "storyboardId"
QT_MOC_LITERAL(7, 50, 5), // "total"
QT_MOC_LITERAL(8, 56, 9), // "completed"
QT_MOC_LITERAL(9, 66, 6), // "failed"
QT_MOC_LITERAL(10, 73, 9), // "createdAt"
QT_MOC_LITERAL(11, 83, 9) // "updatedAt"

    },
    "Job\0id\0type\0status\0JobStatus\0novelId\0"
    "storyboardId\0total\0completed\0failed\0"
    "createdAt\0updatedAt"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_Job[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
      10,   14, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       4,       // flags
       0,       // signalCount

 // properties: name, type, flags
       1, QMetaType::QString, 0x00095103,
       2, QMetaType::QString, 0x00095103,
       3, 0x80000000 | 4, 0x0009510b,
       5, QMetaType::QString, 0x00095103,
       6, QMetaType::QString, 0x00095103,
       7, QMetaType::Int, 0x00095103,
       8, QMetaType::Int, 0x00095103,
       9, QMetaType::Int, 0x00095103,
      10, QMetaType::QDateTime, 0x00095103,
      11, QMetaType::QDateTime, 0x00095103,

       0        // eod
};

void Job::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 2:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< JobStatus >(); break;
        }
    }

#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = reinterpret_cast<Job *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->id(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->type(); break;
        case 2: *reinterpret_cast< JobStatus*>(_v) = _t->status(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->novelId(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->storyboardId(); break;
        case 5: *reinterpret_cast< int*>(_v) = _t->total(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->completed(); break;
        case 7: *reinterpret_cast< int*>(_v) = _t->failed(); break;
        case 8: *reinterpret_cast< QDateTime*>(_v) = _t->createdAt(); break;
        case 9: *reinterpret_cast< QDateTime*>(_v) = _t->updatedAt(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = reinterpret_cast<Job *>(_o);
        Q_UNUSED(_t)
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setId(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setType(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setStatus(*reinterpret_cast< JobStatus*>(_v)); break;
        case 3: _t->setNovelId(*reinterpret_cast< QString*>(_v)); break;
        case 4: _t->setStoryboardId(*reinterpret_cast< QString*>(_v)); break;
        case 5: _t->setTotal(*reinterpret_cast< int*>(_v)); break;
        case 6: _t->setCompleted(*reinterpret_cast< int*>(_v)); break;
        case 7: _t->setFailed(*reinterpret_cast< int*>(_v)); break;
        case 8: _t->setCreatedAt(*reinterpret_cast< QDateTime*>(_v)); break;
        case 9: _t->setUpdatedAt(*reinterpret_cast< QDateTime*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    }
#endif // QT_NO_PROPERTIES
    Q_UNUSED(_o);
}

QT_INIT_METAOBJECT const QMetaObject Job::staticMetaObject = { {
    nullptr,
    qt_meta_stringdata_Job.data,
    qt_meta_data_Job,
    qt_static_metacall,
    nullptr,
    nullptr
} };

QT_WARNING_POP
QT_END_MOC_NAMESPACE
