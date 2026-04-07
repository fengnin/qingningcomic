/****************************************************************************
** Meta object code from reading C++ file 'ImageService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/ImageService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ImageService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_ImageService_t {
    QByteArrayData data[14];
    char stringdata0[185];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_ImageService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_ImageService_t qt_meta_stringdata_ImageService = {
    {
QT_MOC_LITERAL(0, 0, 12), // "ImageService"
QT_MOC_LITERAL(1, 13, 14), // "panelGenerated"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 28), // "ImageService::GenerateResult"
QT_MOC_LITERAL(4, 58, 6), // "result"
QT_MOC_LITERAL(5, 65, 19), // "imageBatchCompleted"
QT_MOC_LITERAL(6, 85, 25), // "ImageService::BatchResult"
QT_MOC_LITERAL(7, 111, 15), // "progressChanged"
QT_MOC_LITERAL(8, 127, 5), // "stage"
QT_MOC_LITERAL(9, 133, 8), // "progress"
QT_MOC_LITERAL(10, 142, 20), // "batchProgressChanged"
QT_MOC_LITERAL(11, 163, 7), // "current"
QT_MOC_LITERAL(12, 171, 5), // "total"
QT_MOC_LITERAL(13, 177, 7) // "message"

    },
    "ImageService\0panelGenerated\0\0"
    "ImageService::GenerateResult\0result\0"
    "imageBatchCompleted\0ImageService::BatchResult\0"
    "progressChanged\0stage\0progress\0"
    "batchProgressChanged\0current\0total\0"
    "message"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_ImageService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       5,    1,   37,    2, 0x06 /* Public */,
       7,    2,   40,    2, 0x06 /* Public */,
      10,    3,   45,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, 0x80000000 | 6,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,    8,    9,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::QString,   11,   12,   13,

       0        // eod
};

void ImageService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ImageService *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->panelGenerated((*reinterpret_cast< const ImageService::GenerateResult(*)>(_a[1]))); break;
        case 1: _t->imageBatchCompleted((*reinterpret_cast< const ImageService::BatchResult(*)>(_a[1]))); break;
        case 2: _t->progressChanged((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->batchProgressChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ImageService::GenerateResult >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ImageService::BatchResult >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ImageService::*)(const ImageService::GenerateResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ImageService::panelGenerated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ImageService::*)(const ImageService::BatchResult & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ImageService::imageBatchCompleted)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ImageService::*)(const QString & , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ImageService::progressChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ImageService::*)(int , int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&ImageService::batchProgressChanged)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject ImageService::staticMetaObject = { {
    QMetaObject::SuperData::link<BatchImageProcessor::staticMetaObject>(),
    qt_meta_stringdata_ImageService.data,
    qt_meta_data_ImageService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *ImageService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImageService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ImageService.stringdata0))
        return static_cast<void*>(this);
    return BatchImageProcessor::qt_metacast(_clname);
}

int ImageService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BatchImageProcessor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void ImageService::panelGenerated(const ImageService::GenerateResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void ImageService::imageBatchCompleted(const ImageService::BatchResult & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void ImageService::progressChanged(const QString & _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ImageService::batchProgressChanged(int _t1, int _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
