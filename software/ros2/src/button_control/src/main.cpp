#include <QApplication>

#include <rclcpp/rclcpp.hpp>
#include "button_control/button_control.hpp"
#include "button_control/main_window.hpp"

int main(int argc, char **argv) {

    rclcpp::init(argc, argv);
    
    auto node = std::make_shared<button_control::ButtonControl>();
    
    std::thread ros_thread([node]() {
        rclcpp::spin(node);
    });
    
    QApplication app(argc, argv);
    
    button_control::MainWindow window(node);
    window.show();
    
    int result = app.exec();
    
    rclcpp::shutdown();
    ros_thread.join();
    
    return result;
    // return 0;
}