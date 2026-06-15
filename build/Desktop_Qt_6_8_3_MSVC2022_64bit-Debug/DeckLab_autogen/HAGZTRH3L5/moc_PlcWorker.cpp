/****************************************************************************
** Meta object code from reading C++ file 'PlcWorker.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../communication/PlcWorker.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'PlcWorker.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN9PlcWorkerE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN9PlcWorkerE = QtMocHelpers::stringData(
    "PlcWorker",
    "motorDataUpdated",
    "",
    "axisId",
    "MotorData",
    "data",
    "sensorDataUpdated",
    "SensorData",
    "connectionStateChanged",
    "connected",
    "errorOccurred",
    "msg",
    "initialize",
    "stop",
    "enableAxis",
    "disableAxis",
    "faultResetAxis",
    "setVelocity",
    "rpm",
    "setPosition",
    "pulses",
    "approachRpm",
    "emergencyStopAll",
    "writeRegister",
    "addr",
    "value",
    "writeRegisters",
    "QList<quint16>",
    "values",
    "pollCycle",
    "tryReconnect"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN9PlcWorkerE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  110,    2, 0x06,    1 /* Public */,
       6,    1,  115,    2, 0x06,    4 /* Public */,
       8,    1,  118,    2, 0x06,    6 /* Public */,
      10,    1,  121,    2, 0x06,    8 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      12,    0,  124,    2, 0x0a,   10 /* Public */,
      13,    0,  125,    2, 0x0a,   11 /* Public */,
      14,    1,  126,    2, 0x0a,   12 /* Public */,
      15,    1,  129,    2, 0x0a,   14 /* Public */,
      16,    1,  132,    2, 0x0a,   16 /* Public */,
      17,    2,  135,    2, 0x0a,   18 /* Public */,
      19,    3,  140,    2, 0x0a,   21 /* Public */,
      22,    0,  147,    2, 0x0a,   25 /* Public */,
      23,    2,  148,    2, 0x0a,   26 /* Public */,
      26,    2,  153,    2, 0x0a,   29 /* Public */,
      29,    0,  158,    2, 0x08,   32 /* Private */,
      30,    0,  159,    2, 0x08,   33 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, 0x80000000 | 4,    3,    5,
    QMetaType::Void, 0x80000000 | 7,    5,
    QMetaType::Void, QMetaType::Bool,    9,
    QMetaType::Void, QMetaType::QString,   11,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::Double,    3,   18,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Double,    3,   20,   21,
    QMetaType::Void,
    QMetaType::Void, QMetaType::UShort, QMetaType::UShort,   24,   25,
    QMetaType::Void, QMetaType::UShort, 0x80000000 | 27,   24,   28,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject PlcWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN9PlcWorkerE.offsetsAndSizes,
    qt_meta_data_ZN9PlcWorkerE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN9PlcWorkerE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<PlcWorker, std::true_type>,
        // method 'motorDataUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<MotorData, std::false_type>,
        // method 'sensorDataUpdated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<SensorData, std::false_type>,
        // method 'connectionStateChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'errorOccurred'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'initialize'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'stop'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'enableAxis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'disableAxis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'faultResetAxis'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setVelocity'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'setPosition'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint32, std::false_type>,
        QtPrivate::TypeAndForceComplete<double, std::false_type>,
        // method 'emergencyStopAll'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'writeRegister'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint16, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint16, std::false_type>,
        // method 'writeRegisters'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<quint16, std::false_type>,
        QtPrivate::TypeAndForceComplete<QVector<quint16>, std::false_type>,
        // method 'pollCycle'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tryReconnect'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void PlcWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PlcWorker *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->motorDataUpdated((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<MotorData>>(_a[2]))); break;
        case 1: _t->sensorDataUpdated((*reinterpret_cast< std::add_pointer_t<SensorData>>(_a[1]))); break;
        case 2: _t->connectionStateChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: _t->initialize(); break;
        case 5: _t->stop(); break;
        case 6: _t->enableAxis((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->disableAxis((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 8: _t->faultResetAxis((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 9: _t->setVelocity((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
        case 10: _t->setPosition((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint32>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3]))); break;
        case 11: _t->emergencyStopAll(); break;
        case 12: _t->writeRegister((*reinterpret_cast< std::add_pointer_t<quint16>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<quint16>>(_a[2]))); break;
        case 13: _t->writeRegisters((*reinterpret_cast< std::add_pointer_t<quint16>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<quint16>>>(_a[2]))); break;
        case 14: _t->pollCycle(); break;
        case 15: _t->tryReconnect(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 13:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<quint16> >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _q_method_type = void (PlcWorker::*)(int , MotorData );
            if (_q_method_type _q_method = &PlcWorker::motorDataUpdated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _q_method_type = void (PlcWorker::*)(SensorData );
            if (_q_method_type _q_method = &PlcWorker::sensorDataUpdated; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _q_method_type = void (PlcWorker::*)(bool );
            if (_q_method_type _q_method = &PlcWorker::connectionStateChanged; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _q_method_type = void (PlcWorker::*)(const QString & );
            if (_q_method_type _q_method = &PlcWorker::errorOccurred; *reinterpret_cast<_q_method_type *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
    }
}

const QMetaObject *PlcWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *PlcWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN9PlcWorkerE.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int PlcWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    }
    return _id;
}

// SIGNAL 0
void PlcWorker::motorDataUpdated(int _t1, MotorData _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void PlcWorker::sensorDataUpdated(SensorData _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void PlcWorker::connectionStateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void PlcWorker::errorOccurred(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
QT_WARNING_POP
