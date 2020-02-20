#ifndef SPHERE_H
#define SPHERE_H

#include <QVector3D>
#include <QDebug>
#include <QQuaternion>
#include <QSizeF>

#include "Object.h"
#include "Collisions.h"

class Sphere : public Object {
private:
    float _radius;
public:
    Sphere(uchar id, QVector3D position, uchar value, uchar size, uchar orientation)
        : Object(id, 1, position, value, size, orientation) {
        switch(size) {
            case 0:
            case 1:
                this->_radius = 0.03;
                break;
            case 2:
            case 3:
                this->_radius = 0.05;
                break;
            case 4:
            case 5:
                this->_radius = 0.08;
                break;
            case 6:
            case 7:
                this->_radius = 0.12;
                break;
        }
    }
    inline float getRadius() { return _radius; }


    inline bool contains(QVector3D point) override {
        float d = point.distanceToPoint(this->_position);

        return d < this->_radius;
    }

    inline QList<QVector3D> getBoundingBox() override {
        QList<QVector3D> list;

        list.append(this->_position + QVector3D(this->_radius, 0, 0));
        list.append(this->_position - QVector3D(this->_radius, 0, 0));
        list.append(this->_position + QVector3D(0, this->_radius, 0));
        list.append(this->_position - QVector3D(0, this->_radius, 0));
        list.append(this->_position + QVector3D(0, 0, this->_radius));
        list.append(this->_position - QVector3D(0, 0, this->_radius));

        return list;
    }
};

#endif // SPHERE_H
