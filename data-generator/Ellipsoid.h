#ifndef ELLIPSOID_H
#define ELLIPSOID_H

#include <QVector3D>
#include <QDebug>
#include <QQuaternion>
#include <QSizeF>

#include "Object.h"

class Ellipsoid : public Object {
private:
    QVector3D _size;
public:
    Ellipsoid(uchar id, QVector3D position, char value, uchar size, uchar orientation)
        : Object(id, 2, position, value, size, orientation) {
        switch(size) {
            case 0:
            case 1:
            case 2:
                this->_size = QVector3D(0.05, 0.2, 0.07);
                break;
            case 3:
            case 4:
                this->_size = QVector3D(0.02, 0.1, 0.03);
                break;
            case 5:
            case 6:
            case 7:
                this->_size = QVector3D(0.07, 0.08, 0.1);
                break;        
        }
    }
    //inline QSizeF getDimensions() { return _size; }

    inline bool contains(QVector3D point) override {
        auto tp = point - this->_position;
        tp = this->_rotation.inverted().rotatedVector(tp);
        tp += this->_position;


        float a = ((point.x() - this->_position.x()) / this->_size.x());
        float b = ((point.y() - this->_position.y()) / this->_size.y());
        float c = ((point.z() - this->_position.z()) / this->_size.z());

        return ((a*a) + (b*b) + (c*c)) < 1;
    }

    inline QList<QVector3D> getBoundingBox() override {
        QList<QVector3D> list;

        list.append(this->_position + QVector3D(this->_size.x(), 0, 0));
        list.append(this->_position - QVector3D(this->_size.x(), 0, 0));
        list.append(this->_position + QVector3D(0, this->_size.y(), 0));
        list.append(this->_position - QVector3D(0, this->_size.y(), 0));
        list.append(this->_position + QVector3D(0, 0, this->_size.z()));
        list.append(this->_position - QVector3D(0, 0, this->_size.z()));

        return list;
    }
};

#endif // ELLIPSOID_H
