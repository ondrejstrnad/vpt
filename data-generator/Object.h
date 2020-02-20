#ifndef OBJECT_H
#define OBJECT_H

#include <QVector3D>
#include <QDebug>
#include <QQuaternion>
#include <QSizeF>

class Object {
protected:
    QVector3D _position;
    QQuaternion _rotation;

    uchar _id;
    uchar _value;
    uchar _type; // 0=undefined, 1=sphere, 2=ellipsoid, 3=box
    uchar _size; // 8 size classes
    uchar _orientation; // 8 possible orientations
public:
    Object(uchar id, uchar type, QVector3D position, uchar value, uchar size, uchar orientation) {
        this->_id = id;
        this->_position = position;
        this->_value = value;
        this->_type = type;
        this->_orientation = orientation;
        this->_size = size;
    }

    inline QVector3D getPosition() { return _position; }
    inline QQuaternion getRotation() { return _rotation; } // probably not useful

    // for volumetric data
    inline uchar getId() { return _id; }
    inline uchar getValue() { return _value; }
    inline uchar getType() { return _type; }
    inline uchar getSize() { return _size; }
    inline uchar getOrientation() { return _orientation; }

    virtual bool contains(QVector3D point) = 0;
    virtual QList<QVector3D> getBoundingBox() = 0;
};
// ===================================

#endif // OBJECT_H
