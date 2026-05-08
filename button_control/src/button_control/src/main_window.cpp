#include "button_control/main_window.hpp"
#include <QVBoxLayout>
#include <QWidget>

namespace button_control {

MainWindow::MainWindow(std::shared_ptr<ButtonControl> node, QWidget *parent)
: QMainWindow(parent), ros_node_(node) {
    
    // Создаём центральный виджет
    auto *central_widget = new QWidget(this);
    setCentralWidget(central_widget);
    
    // Layout
    auto *layout = new QVBoxLayout(central_widget);
    
    button_start_motors_ = new QPushButton("Start motors", this);
    button_stop_motors_ = new QPushButton("Stop motors", this);
    button_lying_down_ = new QPushButton("Lying down", this);
    button_standing_ = new QPushButton("Standing", this); 
    button_set_zero_pos_ = new QPushButton("Set zero position", this); 

    layout->addWidget(button_start_motors_);
    layout->addWidget(button_stop_motors_);
    layout->addWidget(button_lying_down_);
    layout->addWidget(button_standing_);
    layout->addWidget(button_set_zero_pos_);

    connect(button_start_motors_, &QPushButton::clicked, this, &MainWindow::startMotorsButtonClicked);
    connect(button_stop_motors_, &QPushButton::clicked, this, &MainWindow::stopMotorsButtonClicked);
    connect(button_lying_down_, &QPushButton::clicked, this, &MainWindow::lyingDownButtonClicked);
    connect(button_standing_, &QPushButton::clicked, this, &MainWindow::standingButtonClicked);
    connect(button_set_zero_pos_,&QPushButton::clicked, this, &MainWindow::setZeroPositionButtonClicked);
    
    setWindowTitle("ROS2 + Qt Control Panel");
    resize(500, 300);
}

void MainWindow::lyingDownButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishLyingPose();
    }
}

void MainWindow::standingButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishStandingPose();
    }
}

void MainWindow::setZeroPositionButtonClicked(){

    if (ros_node_) { 
        ros_node_->publishSetZeroCmdMessage();
    }
}

void MainWindow::startMotorsButtonClicked(){

    if (ros_node_) { 
        ros_node_->publishStartMotorsCmdMessage();
    }

}

void MainWindow::stopMotorsButtonClicked(){

    if (ros_node_) { 
        ros_node_->publishStopMotorsCmdMessage();
    }

}

}// namespace button_control
