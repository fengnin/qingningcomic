/****************************************************************************
** Meta object code from reading C++ file 'BibleImageService.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../include/BibleImageService.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'BibleImageService.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_BibleImageService_t {
    QByteArrayData data[16];
    char stringdata0[237];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_BibleImageService_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_BibleImageService_t qt_meta_stringdata_BibleImageService = {
    {
QT_MOC_LITERAL(0, 0, 17), // "BibleImageService"
QT_MOC_LITERAL(1, 18, 17), // "portraitGenerated"
QT_MOC_LITERAL(2, 36, 0), // ""
QT_MOC_LITERAL(3, 37, 11), // "characterId"
QT_MOC_LITERAL(4, 49, 9), // "imagePath"
QT_MOC_LITERAL(5, 59, 23), // "sceneReferenceGenerated"
QT_MOC_LITERAL(6, 83, 7), // "sceneId"
QT_MOC_LITERAL(7, 91, 19), // "bibleBatchCompleted"
QT_MOC_LITERAL(8, 111, 4), // "type"
QT_MOC_LITERAL(9, 116, 12), // "successCount"
QT_MOC_LITERAL(10, 129, 11), // "failedCount"
QT_MOC_LITERAL(11, 141, 23), // "allBibleImagesCompleted"
QT_MOC_LITERAL(12, 165, 16), // "onImageGenerated"
QT_MOC_LITERAL(13, 182, 31), // "QwenImageClient::GenerateResult"
QT_MOC_LITERAL(14, 214, 6), // "result"
QT_MOC_LITERAL(15, 221, 15) // "processNextItem"

    },
    "BibleImageService\0portraitGenerated\0"
    "\0characterId\0imagePath\0sceneReferenceGenerated\0"
    "sceneId\0bibleBatchCompleted\0type\0"
    "successCount\0failedCount\0"
    "allBibleImagesCompleted\0onImageGenerated\0"
    "QwenImageClient::GenerateResult\0result\0"
    "processNextItem"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_BibleImageService[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   44,    2, 0x06 /* Public */,
       5,    2,   49,    2, 0x06 /* Public */,
       7,    3,   54,    2, 0x06 /* Public */,
      11,    2,   61,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      12,    1,   66,    2, 0x08 /* Private */,
      15,    0,   69,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    6,    4,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int,    8,    9,   10,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    9,   10,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 13,   14,
    QMetaType::Void,

       0        // eod
};

void BibleImageService::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<BibleImageService *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->portraitGenerated((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 1: _t->sceneReferenceGenerated((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 2: _t->bibleBatchCompleted((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 3: _t->allBibleImagesCompleted((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->onImageGenerated((*reinterpret_cast< const QwenImageClient::GenerateResult(*)>(_a[1]))); break;
        case 5: _t->processNextItem(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QwenImageClient::GenerateResult >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (BibleImageService::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleImageService::portraitGenerated)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (BibleImageService::*)(const QString & , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleImageService::sceneReferenceGenerated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (BibleImageService::*)(const QString & , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleImageService::bibleBatchCompleted)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (BibleImageService::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&BibleImageService::allBibleImagesCompleted)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject BibleImageService::staticMetaObject = { {
    QMetaObject::SuperData::link<BatchImageProcessor::staticMetaObject>(),
    qt_meta_stringdata_BibleImageService.data,
    qt_meta_data_BibleImageService,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *BibleImageService::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *BibleImageService::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_BibleImageService.stringdata0))
        return static_cast<void*>(this);
    return BatchImageProcessor::qt_metacast(_clname);
}

int BibleImageService::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BatchImageProcessor::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void BibleImageService::portraitGenerated(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void BibleImageService::sceneReferenceGenerated(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void BibleImageService::bibleBatchCompleted(const QString & _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void BibleImageService::allBibleImagesCompleted(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
