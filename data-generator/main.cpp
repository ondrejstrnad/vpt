#include <QVector3D>
#include <QVector4D>
#include <QFile>
#include <QtMath>
#include <QDebug>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "Object.h"
#include "Sphere.h"
#include "Box.h"
#include "Ellipsoid.h"

#include "Collisions.h"

struct Settings {
public:
    // settings
    int w = 128;                        // grid width
    int h = 128;                        // grid height
    int d = 128;                        // grid depth
    int targetCount = 150;              // how many items do we want in the scene
    bool canOverlap = false;            // indication whether the objects can overlap

    // 0=one byte per voxel (value of the voxel),
    // 1=three bytes per voxel (data structure agreed with Ciril's group)
    int outputType = 1;

    QString targetFile;    // target filename

    // what types do we want to include in the generation process (1-sphere, ...)
    QList<uchar> allowedTypes;

    Settings::Settings() {
        allowedTypes.append(1);
        allowedTypes.append(2);
        allowedTypes.append(3);

        targetFile = "data.raw";
    }
};

QByteArray generateData(Settings* set)
{
    // initialization of the seed
    // comment this line out if you want the very same random generator everytime!
    qsrand(QDateTime::currentMSecsSinceEpoch() / 1000);


    // generate a bunch of objects
    QList<Object*> objects;

    while(objects.size() < set->targetCount) {
        // random position
        float x = (qrand() % 100) * 0.01f;
        float y = (qrand() % 100) * 0.01f;
        float z = (qrand() % 100) * 0.01f;

        uchar type = set->allowedTypes[qrand() % set->allowedTypes.size()];
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
        if(!set->canOverlap) {
            for(int i = 0; i < objects.size(); i++) {
                if(Collisions::intersect(obj, objects[i])) {
                    collision = true;
                    break;
                }
            }
        }

        if(set->canOverlap || !collision) {
            objects.append(obj);

            qDebug() << obj << " " << obj->getPosition() << " " << obj->getSize();
        } else {
            delete obj;
        }        
    }

    qDebug() << objects.size() << " objects generated";

    // generate the data as a byte array
    float partX = 1.0f / set->w;
    float partY = 1.0f / set->h;
    float partZ = 1.0f / set->d;

    QByteArray data;
    int counter = 0;

    // rasterizing grid
    Object* latest = nullptr;
    for(int z = 0; z < set->d; z++) {
        for(int x = 0; x < set->w; x++) {
            for(int y = 0; y < set->h; y++) {
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

                    switch(set->outputType) {
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
                    switch(set->outputType) {
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

QByteArray generateMeta(Settings* set)
{
    QJsonObject root;

    QJsonObject general;
    general["info"] = "Binary file contains synthetic volumetric data for VPT renderer.";
    general["particles"] = set->targetCount;
    general["width"] = set->w;
    general["height"] = set->h;
    general["depth"] = set->d;
    general["bits"] = set->outputType == 0 ? 8 : 32;
    root["general"] = general;

    QJsonArray layout;
    if(set->outputType == 0) {
        QJsonObject value;
        value["name"] = "Value";
        value["bits"] = 8;
        value["datatype"] = "byte";
        value["desc"] = "Value of the element presented in the current cell.";
        layout.append(value);
    } else {
        QJsonObject header;
        header["name"] = "Header";
        header["bits"] = 8;
        header["datatype"] = "complex";
        header["desc"] = "Header byte of a cell.";

        QJsonObject type;
        type["name"] = "Type";
        type["bits"] = 2;
        type["datatype"] = "enum";
        type["desc"] = "Type of the element";
        QJsonArray values;
        values.append("Undefined");
        values.append("Sphere");
        values.append("Ellipsoid");
        values.append("Box");
        type["values"] = values;

        QJsonObject size;
        size["name"] = "Size";
        size["bits"] = 3;
        size["datatype"] = "enum";
        size["desc"] = "Type of the element";
        QJsonArray valuesS;
        valuesS.append("Class 1");
        valuesS.append("Class 2");
        valuesS.append("Class 3");
        valuesS.append("Class 4");
        valuesS.append("Class 5");
        valuesS.append("Class 6");
        valuesS.append("Class 7");
        valuesS.append("Class 8");
        size["values"] = valuesS;

        QJsonObject orientation;
        orientation["name"] = "Orientation";
        orientation["bits"] = 3;
        orientation["datatype"] = "enum";
        orientation["desc"] = "Type of the element";
        QJsonArray valuesO;
        valuesO.append("Random");
        valuesO.append("Front");
        valuesO.append("Left");
        valuesO.append("Up");
        valuesO.append("Down");
        valuesO.append("Back");
        valuesO.append("Diagonal");
        valuesO.append("Random");
        orientation["values"] = valuesO;

        QJsonArray layoutH;
        layoutH.append(type);
        layoutH.append(size);
        layoutH.append(orientation);
        header["layout"] = layoutH;

        layout.append(header);

        QJsonObject id;
        id["name"] = "ID";
        id["bits"] = 8;
        id["datatype"] = "byte";
        id["desc"] = "ID of the element presented in the current cell.";
        layout.append(id);

        QJsonObject value;
        value["name"] = "Value";
        value["bits"] = 8;
        value["datatype"] = "byte";
        value["desc"] = "Value of the element presented in the current cell.";
        layout.append(value);

        QJsonObject padding;
        padding["name"] = "Padding";
        padding["bits"] = 8;
        padding["datatype"] = "byte";
        padding["desc"] = "Zeros used for padding.";
        layout.append(padding);
    }
    root["layout"] = layout;

    QJsonDocument doc(root);
    return doc.toJson();
}

void writeData(QByteArray data, Settings* set) {

    // write data into the file

    QFile file(set->targetFile);

    qDebug() << "written to: " << QFileInfo(file).absoluteFilePath();

    file.open(QIODevice::WriteOnly);

    file.write(data);

    file.close();
}

int main(int argc, char *argv[])
{
    //qDebug() << (uchar)(1 << 6);

    Settings set;
    set.outputType = 1;

    // main data generator
    QByteArray data = generateData(&set);
    writeData(data, &set);

    // meta file descriptor
    data = generateMeta(&set);
    set.targetFile = "data.json";
    writeData(data, &set);

    return 0;
}
