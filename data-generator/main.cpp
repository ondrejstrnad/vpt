#include <QVector3D>
#include <QVector4D>
#include <QFile>
#include <QtMath>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>

#include "Object.h"
#include "Sphere.h"
#include "Box.h"
#include "Ellipsoid.h"

#include "Collisions.h"


QByteArray generateData()
{
    // initialization of the seed
    // comment this line out if you want the very same random generator everytime!
    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);

    // settings
    int w = 128;                        // grid width
    int h = 128;                        // grid height
    int d = 128;                        // grid depth
    int targetCount = 150;               // how many items do we want in the scene
    bool canOverlap = false;            // indication whether the objects can overlap    

    // 0=one byte per voxel (value of the voxel),
    // 1=three bytes per voxel (data structure agreed with Ciril's group)
    int outputType = 1;

    // what types do we want to include in the generation process (1-sphere, ...)
    QList<uchar> allowedTypes;
    allowedTypes.append(1);
    allowedTypes.append(2);
    allowedTypes.append(3);

    // END settings


    // generate a bunch of objects
    QList<Object*> objects;

    while(objects.size() < targetCount) {
        // random position
        float x = (qrand() % 100) * 0.01f;
        float y = (qrand() % 100) * 0.01f;
        float z = (qrand() % 100) * 0.01f;

        uchar type = allowedTypes[qrand() % allowedTypes.size()];
        uchar size = qrand() % 8; // 8 possible size classes
        uchar orientation = qrand() % 8; // 8 possible orientations
        uchar value = size * 32;
        uchar id = objects.size() + 1;

        Object* obj = nullptr;
        switch (type) {
            case 1:
                obj = new Sphere(id, QVector3D(x, y, z), value, size, orientation);
                break;
            case 2:
                obj = new Ellipsoid(id, QVector3D(x, y, x), value, size, orientation);
                break;
            case 3:
                obj = new Box(id, QVector3D(x,y,z), value, size, orientation);
                break;
        }

        // check the collision
        bool collision = false;
        if(!canOverlap) {
            for(int i = 0; i < objects.size(); i++) {
                if(Collisions::intersect(obj, objects[i])) {
                    collision = true;
                    break;
                }
            }
        }

        if(canOverlap || !collision) {
            objects.append(obj);

            qDebug() << obj << " " << obj->getPosition() << " " << obj->getSize();
        } else {
            delete obj;
        }        
    }

    qDebug() << objects.size() << " objects generated";

    // generate the data as a byte array
    float partX = 1.0f / w;
    float partY = 1.0f / h;
    float partZ = 1.0f / d;

    QByteArray data;
    int counter = 0;

    // rasterizing grid
    Object* latest = nullptr;
    for(int z = 0; z < d; z++) {
        for(int x = 0; x < w; x++) {
            for(int y = 0; y < h; y++) {
                auto center = QVector3D(x * partX + partX * 0.5f, y * partY + partY * 0.5f, z * partZ + partZ * 0.5f);

                Object* obj = latest;

                if(obj != nullptr && obj->contains(center)) {
                    // speeding up ... don't have to go through all the objects again
                } else {
                    obj = nullptr;
                    for(int i = 0; i < objects.size(); i++) {
                        if(objects[i]->contains(center)) {
                            obj = objects[i];
                            break;
                        }
                    }
                    latest = obj;
                }

                uchar meta = 0;

                if(obj != nullptr) {

                    //qDebug() << "type: " << obj->getType();
                    //qDebug() << "size: " << obj->getSize();
                    //qDebug() << "orientation: " << obj->getOrientation();

                    meta = obj->getOrientation();
                    meta |= ((uchar)obj->getSize() << 3);
                    meta |= ((uchar)obj->getType() << 6);

                    //qDebug() << "meta: " << meta;

                    switch(outputType) {
                        case 0:
                            data.push_back(obj->getValue());
                            break;
                        case 1:
                            data.push_back(meta);
                            data.push_back(obj->getId());
                            data.push_back(obj->getValue());
                            data.push_back((char)0); // padding
                            break;
                    }
                } else {
                    switch(outputType) {
                        case 0:
                        data.push_back(meta);
                        break;
                        case 1:
                            data.push_back(meta);
                            data.push_back(meta);
                            data.push_back(meta);
                            data.push_back(meta); // padding
                        break;
                    }
                }
            }            
        }        
    }


    return data;
}

void writeData(QByteArray data) {

    // write data into the file
    QString targetFile = "data.raw";    // target filename
    QFile file(targetFile);

    qDebug() << "written to: " << QFileInfo(file).absoluteFilePath();

    file.open(QIODevice::WriteOnly);

    file.write(data);

    file.close();
}

int main(int argc, char *argv[])
{
    //qDebug() << (uchar)(1 << 6);

    auto data = generateData();

    writeData(data);

    return 0;
}
