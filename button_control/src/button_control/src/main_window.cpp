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
    
    // Кнопка
    button_move_zero_pos_ = new QPushButton("Move to zero position", this);
    button_lying_down_ = new QPushButton("Liyng down", this);
    button_standing_ = new QPushButton("Standing", this); 
    button_set_zero_pos_ = new QPushButton("Set zero position", this); 
    button_activate_motors_ = new QPushButton("Activate motors", this); 


    layout->addWidget(button_move_zero_pos_);
    layout->addWidget(button_lying_down_);
    layout->addWidget(button_standing_);
    layout->addWidget(button_set_zero_pos_);
    layout->addWidget(button_activate_motors_);

    
    // Сигнал-слот
    connect(button_move_zero_pos_, &QPushButton::clicked, this, &MainWindow::moveZeroPositionButtonClicked);
    connect(button_lying_down_, &QPushButton::clicked, this, &MainWindow::lyingDownButtonClicked);
    connect(button_standing_, &QPushButton::clicked, this, &MainWindow::standingButtonClicked);
    connect(button_set_zero_pos_,&QPushButton::clicked, this, &MainWindow::setZeroPositionButtonClicked);
    connect(button_activate_motors_,&QPushButton::clicked, this, &MainWindow::activateMotorsButtonClicked);
    
    setWindowTitle("ROS2 + Qt Control Panel");
    resize(500, 300);
}

void MainWindow::moveZeroPositionButtonClicked() {

    if (ros_node_) {

        ros_node_->publishMoveZeroCmdMessage();
    
    }   
} 
void MainWindow::lyingDownButtonClicked() {

    // if (ros_node_) {
    //     tinker_msgs::msg::LowCmd low_cmd_msg;
    //     control_cmd_msg.motor_id = 0; 
    //     control_cmd_msg.cmd = 252;    
    //     ros_node_->publishControlCmdMessage(control_cmd_msg);
    
    // }   
} 
void MainWindow::standingButtonClicked() {

    if (ros_node_) {
        // tinker_msgs::msg::ControlCmd control_cmd_msg;
        // control_cmd_msg.motor_id = 0; 
        // control_cmd_msg.cmd = 252;    
        // ros_node_->publishControlCmdMessage(control_cmd_msg);
    
    }   
} 

void MainWindow::setZeroPositionButtonClicked(){

    if (ros_node_) { 
        ros_node_->publishSetZeroCmdMessage();
    }
}

void MainWindow::activateMotorsButtonClicked(){

    if (ros_node_) { 
        ros_node_->publishActivateMotorsCmdMessage();
    }

}

}// namespace button_control