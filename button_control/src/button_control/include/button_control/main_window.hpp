#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>
#include <QLabel>

#include <memory>
#include "button_control.hpp"

namespace button_control {

class MainWindow : public QMainWindow {
    Q_OBJECT
    
public:
    explicit MainWindow(std::shared_ptr<ButtonControl> node, QWidget *parent = nullptr);
    
private slots:
    void lyingDownButtonClicked();
    void standingButtonClicked();
    void setZeroPositionButtonClicked();
    void startMotorsButtonClicked();
    void stopMotorsButtonClicked();
    
private:
    std::shared_ptr<ButtonControl> ros_node_;
    QPushButton *button_start_motors_;
    QPushButton *button_stop_motors_;
    QPushButton *button_lying_down_;
    QPushButton *button_standing_;
    QPushButton *button_set_zero_pos_;

    QLabel * label_status_;

};

} // namespace button_control

#endif
