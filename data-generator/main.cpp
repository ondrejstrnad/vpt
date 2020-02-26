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
#include <QDataStream>

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
    // 1=three bytes per voxel (data structure agreed with Ciril's group) + one byte for padding
    // 2=five floats per voxel
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

QList<Object*> generateObjects(Settings* set)
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

            qDebug() << obj->getName() << " " << obj->getPosition() << " " << obj->getSize();
        } else {
            delete obj;
        }
    }

    qDebug() << objects.size() << " objects generated";

    return objects;
}

QByteArray generateData(QList<Object*> objects, Settings* set)
{     
    // generate the data as a byte array
    float partX = 1.0f / set->w;
    float partY = 1.0f / set->h;
    float partZ = 1.0f / set->d;

    QByteArray data;
    QDataStream out(&data, QIODevice::OpenModeFlag::ReadWrite);
    out.setFloatingPointPrecision(QDataStream::SinglePrecision);
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
                            //data.push_back(obj->getValue());

                            out << obj->getValue();
                            break;
                        case 1:
                            //data.push_back(meta);
                            //data.push_back(obj->getId());
                            //data.push_back(obj->getValue());
                            //data.push_back((char)0); // padding

                            out << meta;
                            out << obj->getId();
                            out << obj->getValue();
                            out << (uchar)0;
                            break;
                        case 2:
                            out << (float)obj->getType();
                            out << (float)obj->getSize();
                            out << (float)obj->getOrientation();
                            out << (float)obj->getId();
                            out << (float)obj->getValue();
                            break;
                    }
                } else {
                    switch(set->outputType) {
                        case 0:
                            //data.push_back(meta);

                            out << meta;
                        break;
                        case 1:
                            for(int i = 0; i < 4; i++) {
                                //data.push_back(meta);

                                out << meta;
                            }
                        break;
                        case 2:
                            for(int i = 0; i < 5; i++) {
                                out << (float)meta;
                            }
                            break;
                    }
                }
            }            
        }        
    }



    return data;
}

QJsonObject computeStats(QList<Object*> objects)
{
    QJsonObject stats;
    QJsonObject global;
    QJsonObject elements;

    QJsonObject type;
    int tsc = 0, tbc = 0, tec = 0;

    QJsonObject size;
    int sc[8];
    int tssc[8], tbsc[8], tesc[8];

    QJsonObject orientation;
    int oc[8];
    int tsoc[8], tboc[8], teoc[8];

    for(int i = 0; i < 8; i++) {
        sc[i] = oc[i] = 0;
        tssc[i] = tbsc[i] = tesc[i] = 0;
        tsoc[i] = tboc[i] = teoc[i] = 0;
    }

    for(auto o : objects) {
        switch(o->getType()) {
            case 1:
                tsc++;
                tsoc[o->getOrientation()]++;
                tssc[o->getSize()]++;
                break;
            case 2:
                tbc++;
                tboc[o->getOrientation()]++;
                tbsc[o->getSize()]++;
                break;
            case 3:
                tec++;
                teoc[o->getOrientation()]++;
                tesc[o->getSize()]++;
                break;
        }

        oc[o->getOrientation()]++;
        sc[o->getSize()]++;
    }

    type["Sphere"] = tsc;
    type["Box"] = tbc;
    type["Ellipsoid"] = tec;
    global["Type"] = type;

    for(int i = 0; i < 8; i++) {
        size["Class " + QString::number(i + 1)] = sc[i];
    }
    global["Size"] = size;

    orientation["Random"] = oc[0];
    orientation["Front"] = oc[1];
    orientation["Left"] = oc[2];
    orientation["Up"] = oc[3];
    orientation["Down"] = oc[4];
    orientation["Back"] = oc[5];
    orientation["Diagonal"] = oc[6];
    orientation["InverseDiagonal"] = oc[7];
    global["Orientation"] = orientation;

    stats["global"] = global;

    QJsonObject typeS, typeB, typeE;
    QJsonObject sizeS, sizeB, sizeE;
    QJsonObject orientationS, orientationB, orientationE;

    // sphere
    for(int i = 0; i < 8; i++) {
        sizeS["Class " + QString::number(i + 1)] = tssc[i];
    }

    orientationS["Random"] = tsoc[0];
    orientationS["Front"] = tsoc[1];
    orientationS["Left"] = tsoc[2];
    orientationS["Up"] = tsoc[3];
    orientationS["Down"] = tsoc[4];
    orientationS["Back"] = tsoc[5];
    orientationS["Diagonal"] = tsoc[6];
    orientationS["InverseDiagonal"] = tsoc[7];

    typeS["Size"] = sizeS;
    typeS["Orientation"] = orientationS;

    elements["Sphere"] = typeS;

    // box
    for(int i = 0; i < 8; i++) {
        sizeB["Class " + QString::number(i + 1)] = tbsc[i];
    }

    orientationB["Random"] = tboc[0];
    orientationB["Front"] = tboc[1];
    orientationB["Left"] = tboc[2];
    orientationB["Up"] = tboc[3];
    orientationB["Down"] = tboc[4];
    orientationB["Back"] = tboc[5];
    orientationB["Diagonal"] = tboc[6];
    orientationB["InverseDiagonal"] = tboc[7];

    typeB["Size"] = sizeB;
    typeB["Orientation"] = orientationB;

    elements["Box"] = typeB;

    // ellipsoid
    for(int i = 0; i < 8; i++) {
        sizeE["Class " + QString::number(i + 1)] = tesc[i];
    }

    orientationE["Random"] = teoc[0];
    orientationE["Front"] = teoc[1];
    orientationE["Left"] = teoc[2];
    orientationE["Up"] = teoc[3];
    orientationE["Down"] = teoc[4];
    orientationE["Back"] = teoc[5];
    orientationE["Diagonal"] = teoc[6];
    orientationE["InverseDiagonal"] = teoc[7];

    typeE["Size"] = sizeE;
    typeE["Orientation"] = orientationE;

    elements["Ellipsoid"] = typeE;

    stats["elements"] = elements;

    return stats;
}

QByteArray generateMeta(QList<Object*> objects, Settings* set)
{
    QJsonObject root;

    QJsonObject general;
    general["info"] = "Binary file contains synthetic volumetric data for VPT renderer.";
    general["width"] = set->w;
    general["height"] = set->h;
    general["depth"] = set->d;

    switch(set->outputType) {
        case 0:
            general["bits"] = 8;
            break;
        case 1:
            general["bits"] = 32;
            break;
        case 2:
            general["bits"] = 160;
            break;
    }

    general["particles"] = set->targetCount;

    root["general"] = general;
    root["stats"] = computeStats(objects);

    QJsonArray layout, values, valuesS, valuesO, layoutH;
    QJsonObject value, header, type, size, orientation, id, padding;
    switch(set->outputType) {
        case 0:
            value["name"] = "Value";
            value["bits"] = 8;
            value["datatype"] = "byte";
            value["desc"] = "Value of the element presented in the current cell.";
            layout.append(value);
            break;
        case 1:
            header["name"] = "Header";
            header["bits"] = 8;
            header["datatype"] = "complex";
            header["desc"] = "Header byte of a cell.";

            type["name"] = "Type";
            type["bits"] = 2;
            type["datatype"] = "enum";
            type["desc"] = "Type of the element";
            values.append("Undefined");
            values.append("Sphere");
            values.append("Ellipsoid");
            values.append("Box");
            type["values"] = values;

            size["name"] = "Size";
            size["bits"] = 3;
            size["datatype"] = "enum";
            size["desc"] = "Size of the element";
            valuesS.append("Class 1");
            valuesS.append("Class 2");
            valuesS.append("Class 3");
            valuesS.append("Class 4");
            valuesS.append("Class 5");
            valuesS.append("Class 6");
            valuesS.append("Class 7");
            valuesS.append("Class 8");
            size["values"] = valuesS;

            orientation["name"] = "Orientation";
            orientation["bits"] = 3;
            orientation["datatype"] = "enum";
            orientation["desc"] = "Orientation of the element";
            valuesO.append("Random");
            valuesO.append("Front");
            valuesO.append("Left");
            valuesO.append("Up");
            valuesO.append("Down");
            valuesO.append("Back");
            valuesO.append("Diagonal");
            valuesO.append("InverseDiagonal");
            orientation["values"] = valuesO;

            layoutH.append(type);
            layoutH.append(size);
            layoutH.append(orientation);
            header["layout"] = layoutH;

            layout.append(header);

            id["name"] = "ID";
            id["bits"] = 8;
            id["datatype"] = "byte";
            id["desc"] = "ID of the element presented in the current cell.";
            layout.append(id);

            value["name"] = "Value";
            value["bits"] = 8;
            value["datatype"] = "byte";
            value["desc"] = "Value of the element presented in the current cell.";
            layout.append(value);

            padding["name"] = "Padding";
            padding["bits"] = 8;
            padding["datatype"] = "byte";
            padding["desc"] = "Zeros used for padding.";
            layout.append(padding);
        break;
        case 2:
            type["name"] = "Type";
            type["bits"] = 32;
            type["datatype"] = "float";
            type["desc"] = "Type of the element";
            values.append("Undefined");
            values.append("Sphere");
            values.append("Ellipsoid");
            values.append("Box");
            type["values"] = values;

            size["name"] = "Size";
            size["bits"] = 32;
            size["datatype"] = "float";
            size["desc"] = "Size of the element";
            valuesS.append("Class 1");
            valuesS.append("Class 2");
            valuesS.append("Class 3");
            valuesS.append("Class 4");
            valuesS.append("Class 5");
            valuesS.append("Class 6");
            valuesS.append("Class 7");
            valuesS.append("Class 8");
            size["values"] = valuesS;

            orientation["name"] = "Orientation";
            orientation["bits"] = 32;
            orientation["datatype"] = "float";
            orientation["desc"] = "Orientation of the element";
            valuesO.append("Random");
            valuesO.append("Front");
            valuesO.append("Left");
            valuesO.append("Up");
            valuesO.append("Down");
            valuesO.append("Back");
            valuesO.append("Diagonal");
            valuesO.append("InverseDiagonal");
            orientation["values"] = valuesO;

            layout.append(type);
            layout.append(size);
            layout.append(orientation);

            id["name"] = "ID";
            id["bits"] = 32;
            id["datatype"] = "float";
            id["desc"] = "ID of the element presented in the current cell.";
            layout.append(id);

            value["name"] = "Value";
            value["bits"] = 32;
            value["datatype"] = "float";
            value["desc"] = "Value of the element presented in the current cell.";
            layout.append(value);
        break;
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
    // setting of the generator
    Settings set;
    set.w = 128;
    set.h = 128;
    set.d = 128;
    set.targetCount = 150;
    set.outputType = 2;

    // main data generator
    QList<Object*> objects = generateObjects(&set);
    QByteArray data = generateData(objects, &set);
    writeData(data, &set);

    // meta file descriptor
    data = generateMeta(objects, &set);
    set.targetFile = "data.json";
    writeData(data, &set);

    return 0;
}
