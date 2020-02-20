#ifndef BOX_H
#define BOX_H

#include <QVector3D>
#include <QDebug>
#include <QQuaternion>
#include <QSizeF>

#include "Object.h"

class Box : public Object {
private:
    QVector3D _size;
public:
    Box(uchar id, QVector3D position, char value, uchar size, uchar orientation)
        : Object(id, 3, position, value, size, orientation) {

        switch(size) {
            case 0:
            case 1:
            case 2:
                this->_size = QVector3D(0.03, 0.06, 0.09);
                break;
            case 3:                
            case 4:
            case 5:
                this->_size = QVector3D(0.11, 0.05, 0.04);
                break;
            case 6:
            case 7:
                this->_size = QVector3D(0.05, 0.12, 0.2);
                break;        
        }

        switch(orientation) {
            default:
            case 0: // random rotation
            case 7:
                this->_rotation = QQuaternion::fromEulerAngles(qrand() % 360 - 180, qrand() % 360 - 180, qrand() % 360 - 180);
                break;
            case 1: // no rotation // front - yaw
                this->_rotation = QQuaternion::fromEulerAngles(0, 0, 0);
                break;
            case 2: // left - roll
                this->_rotation = QQuaternion::fromEulerAngles(0, 0, 90);
                break;
            case 3: // up - pitch
                this->_rotation = QQuaternion::fromEulerAngles(90, 0, 0);
                break;
            case 4: // down - pitch
                this->_rotation = QQuaternion::fromEulerAngles(-90, 0, 0);
                break;
            case 5: // back - yaw
                this->_rotation = QQuaternion::fromEulerAngles(0, -180, 0);
                break;
            case 6: // diagonal
                this->_rotation = QQuaternion::fromEulerAngles(45, 45, 45);
                break;
        }

    }
    //inline QVector3D getDimensions() { return _size; }

    inline bool contains(QVector3D point) override {

        auto tp = point - this->_position;
        tp = this->_rotation.inverted().rotatedVector(tp);
        tp += this->_position;

        float xmin = this->_position.x() - this->_size.x() * 0.5f;
        float xmax = this->_position.x() + this->_size.x() * 0.5f;
        float ymin = this->_position.y() - this->_size.y() * 0.5f;
        float ymax = this->_position.y() + this->_size.y() * 0.5f;
        float zmin = this->_position.z() - this->_size.z() * 0.5f;
        float zmax = this->_position.z() + this->_size.z() * 0.5f;

        return xmin <= tp.x() && tp.x() <= xmax &&
               ymin <= tp.y() && tp.y() <= ymax &&
               zmin <= tp.z() && tp.z() <= zmax;
    }

    inline QList<QVector3D> getBoundingBox() override {
        QList<QVector3D> list;

        float xmin = this->_position.x() - this->_size.x() * 0.5f;
        float xmax = this->_position.x() + this->_size.x() * 0.5f;
        float ymin = this->_position.y() - this->_size.y() * 0.5f;
        float ymax = this->_position.y() + this->_size.y() * 0.5f;
        float zmin = this->_position.z() - this->_size.z() * 0.5f;
        float zmax = this->_position.z() + this->_size.z() * 0.5f;

        list.append(this->_rotation.rotatedVector(QVector3D(xmin, ymin, zmin)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmax, ymin, zmin)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmax, ymin, zmax)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmin, ymin, zmax)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmin, ymax, zmin)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmax, ymax, zmin)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmax, ymax, zmax)));
        list.append(this->_rotation.rotatedVector(QVector3D(xmin, ymax, zmax)));

        return list;
    }
};

#endif // BOX_H
