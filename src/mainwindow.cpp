#include "mainwindow.h"
#include "settings.h"

#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>
#include <iostream>

void MainWindow::initialize()
{
    realtime = new Realtime;
    aspectRatioWidget = new AspectRatioWidget(this);
    aspectRatioWidget->setAspectWidget(realtime, 3.f / 4.f);

    QHBoxLayout *hLayout = new QHBoxLayout;
    QVBoxLayout *vLayout = new QVBoxLayout();
    vLayout->setAlignment(Qt::AlignTop);
    hLayout->addLayout(vLayout);
    hLayout->addWidget(aspectRatioWidget, 1);
    this->setLayout(hLayout);

    QFont font;
    font.setPointSize(12);
    font.setBold(true);

    QLabel *tesselation_label = new QLabel();
    tesselation_label->setText("Parameters");
    tesselation_label->setFont(font);

    QLabel *filters_label = new QLabel();
    filters_label->setText("Interaction");
    filters_label->setFont(font);

    QLabel *ec_label = new QLabel();
    ec_label->setText("Modes");
    ec_label->setFont(font);

    QLabel *param1_label = new QLabel();
    param1_label->setText("Explosion strength");

    QLabel *param2_label = new QLabel();
    param2_label->setText("Oribitting speed");


    filter1 = new QCheckBox();
    filter1->setText(QStringLiteral("Attraction model"));
    filter1->setChecked(false);

    // filter2 = new QCheckBox();
    // filter2->setText(QStringLiteral("Solar System"));
    // filter2->setChecked(false);

    uploadFile = new QPushButton();
    uploadFile->setText(QStringLiteral("Upload Scene File"));

    saveImage = new QPushButton();
    saveImage->setText(QStringLiteral("Save image"));

    QGroupBox *p1Layout = new QGroupBox();
    QHBoxLayout *l1 = new QHBoxLayout();
    QGroupBox *p2Layout = new QGroupBox();
    QHBoxLayout *l2 = new QHBoxLayout();

    p1Slider = new QSlider(Qt::Orientation::Horizontal);
    p1Slider->setTickInterval(5);
    p1Slider->setMinimum(25);
    p1Slider->setMaximum(200);
    p1Slider->setValue(25);

    p1Box = new QSpinBox();
    p1Box->setMinimum(25);
    p1Box->setMaximum(200);
    p1Box->setSingleStep(5);
    p1Box->setValue(25);

    p2Slider = new QSlider(Qt::Orientation::Horizontal);
    p2Slider->setTickInterval(1);
    p2Slider->setMinimum(1);
    p2Slider->setMaximum(25);
    p2Slider->setValue(1);

    p2Box = new QSpinBox();
    p2Box->setMinimum(1);
    p2Box->setMaximum(25);
    p2Box->setSingleStep(1);
    p2Box->setValue(1);

    l1->addWidget(p1Slider);
    l1->addWidget(p1Box);
    p1Layout->setLayout(l1);

    l2->addWidget(p2Slider);
    l2->addWidget(p2Box);
    p2Layout->setLayout(l2);

    ec1 = new QCheckBox();
    ec1->setText(QStringLiteral("Earth Surface/ ESC"));
    ec1->setChecked(false);

    ec2 = new QCheckBox();
    ec2->setText(QStringLiteral("Explosion"));
    ec2->setChecked(false);

    ec3 = new QCheckBox();
    ec3->setText(QStringLiteral("Celestial gravity"));
    ec3->setChecked(false);

    ec4 = new QCheckBox();
    ec4->setText(QStringLiteral("Liquid mode"));
    ec4->setChecked(false);

    vLayout->addWidget(uploadFile);
    vLayout->addWidget(saveImage);
    vLayout->addWidget(tesselation_label);
    vLayout->addWidget(param1_label);
    vLayout->addWidget(p1Layout);
    vLayout->addWidget(param2_label);
    vLayout->addWidget(p2Layout);


    vLayout->addWidget(filters_label);
    vLayout->addWidget(filter1);
    // vLayout->addWidget(filter2);

    vLayout->addWidget(ec_label);
    vLayout->addWidget(ec1);
    vLayout->addWidget(ec2);
    vLayout->addWidget(ec3);
    vLayout->addWidget(ec4);

    connectUIElements();


    onValChangeP1(25);
    onValChangeP2(5);

    // Setting NF
    settings.nearPlane = 0.1f;
    settings.farPlane = 10.f;

}

void MainWindow::finish()
{
    realtime->finish();
    delete (realtime);
}

void MainWindow::connectUIElements()
{
    connectPerPixelFilter();
    // connectKernelBasedFilter();
    connectUploadFile();
    connectSaveImage();
    connectParam1();
    connectParam2();
    connectExtraCredit();

}

void MainWindow::connectParam1()
{
    connect(p1Slider, &QSlider::valueChanged, this, &MainWindow::onValChangeP1);
    connect(p1Box,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &MainWindow::onValChangeP1);
}

void MainWindow::connectParam2()
{
    connect(p2Slider, &QSlider::valueChanged, this, &MainWindow::onValChangeP2);
    connect(p2Box,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &MainWindow::onValChangeP2);
}

void MainWindow::connectPerPixelFilter()
{
    connect(filter1, &QCheckBox::clicked, this, &MainWindow::onPerPixelFilter);
}

void MainWindow::connectKernelBasedFilter()
{
    // connect(filter2, &QCheckBox::clicked, this, &MainWindow::onKernelBasedFilter);
}

void MainWindow::connectUploadFile()
{
    connect(uploadFile, &QPushButton::clicked, this, &MainWindow::onUploadFile);
}

void MainWindow::connectSaveImage()
{
    connect(saveImage, &QPushButton::clicked, this, &MainWindow::onSaveImage);
}

void MainWindow::connectExtraCredit()
{
    connect(ec1, &QCheckBox::clicked, this, &MainWindow::onExtraCredit1);
    connect(ec2, &QCheckBox::clicked, this, &MainWindow::onExtraCredit2);
    connect(ec3, &QCheckBox::clicked, this, &MainWindow::onExtraCredit3);
    connect(ec4, &QCheckBox::clicked, this, &MainWindow::onExtraCredit4);
}

void MainWindow::onPerPixelFilter()
{
    settings.perPixelFilter = filter1->isChecked();
    realtime->settingsChanged();
}

void MainWindow::onKernelBasedFilter()
{
    settings.kernelBasedFilter = filter2->isChecked();
    realtime->settingsChanged();
}

void MainWindow::onUploadFile()
{
    QString configFilePath = QFileDialog::getOpenFileName(this,
                                                          tr("Upload File"),
                                                          QDir::currentPath()
                                                              .append(QDir::separator())
                                                              .append("scenefiles")
                                                              .append(QDir::separator())
                                                              .append("lights-camera")
                                                              .append(QDir::separator())
                                                              .append("required"),
                                                          tr("Scene Files (*.json)"));
    if (configFilePath.isNull()) {
        std::cout << "Failed to load null scenefile." << std::endl;
        return;
    }

    settings.sceneFilePath = configFilePath.toStdString();

    std::cout << "Loaded scenefile: \"" << configFilePath.toStdString() << "\"." << std::endl;

    realtime->sceneChanged();
}

void MainWindow::onSaveImage()
{
    if (settings.sceneFilePath.empty()) {
        std::cout << "No scene file loaded." << std::endl;
        return;
    }
    std::string sceneName = settings.sceneFilePath.substr(0,
                                                          settings.sceneFilePath.find_last_of("."));
    sceneName = sceneName.substr(sceneName.find_last_of("/") + 1);
    QString filePath = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"),
                                                    QDir::currentPath()
                                                        .append(QDir::separator())
                                                        .append("student_outputs")
                                                        .append(QDir::separator())
                                                        .append("lights-camera")
                                                        .append(QDir::separator())
                                                        .append("required")
                                                        .append(QDir::separator())
                                                        .append(sceneName),
                                                    tr("Image Files (*.png)"));
    std::cout << "Saving image to: \"" << filePath.toStdString() << "\"." << std::endl;
    realtime->saveViewportImage(filePath.toStdString());
}

void MainWindow::onValChangeP1(int newValue)
{
    p1Slider->setValue(newValue);
    p1Box->setValue(newValue);
    settings.shapeParameter1 = p1Slider->value();
    realtime->settingsChanged();
}

void MainWindow::onValChangeP2(int newValue)
{
    p2Slider->setValue(newValue);
    p2Box->setValue(newValue);
    settings.shapeParameter2 = p2Slider->value();
    realtime->settingsChanged();
}


void MainWindow::onExtraCredit1()
{
    settings.extraCredit1 = ec1->isChecked();
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit2()
{
    settings.extraCredit2 = ec2->isChecked();
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit3()
{
    settings.extraCredit3 = ec3->isChecked();
    realtime->settingsChanged();
}

void MainWindow::onExtraCredit4()
{
    settings.extraCredit4 = ec4->isChecked();
    realtime->settingsChanged();
}