#include "button_control/main_window.hpp"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

namespace button_control {

namespace {

constexpr const char *kIndicatorRed =
    "background-color: #d32f2f; border-radius: 10px; min-width: 20px; max-width: 20px;"
    "min-height: 20px; max-height: 20px;";
constexpr const char *kIndicatorYellow =
    "background-color: #fbc02d; border-radius: 10px; min-width: 20px; max-width: 20px;"
    "min-height: 20px; max-height: 20px;";
constexpr const char *kIndicatorGreen =
    "background-color: #388e3c; border-radius: 10px; min-width: 20px; max-width: 20px;"
    "min-height: 20px; max-height: 20px;";

}  // namespace

MainWindow::MainWindow(std::shared_ptr<ButtonControl> node, QWidget *parent)
: QMainWindow(parent), ros_node_(std::move(node))
{
    auto *central_widget = new QWidget(this);
    setCentralWidget(central_widget);

    auto *layout = new QVBoxLayout(central_widget);

    auto *motor_row = new QHBoxLayout();
    label_motor_caption_ = new QLabel("Motors", this);
    label_motor_indicator_ = new QLabel(this);
    motor_row->addWidget(label_motor_caption_);
    motor_row->addWidget(label_motor_indicator_);
    motor_row->addStretch();
    layout->addLayout(motor_row);

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
    connect(button_set_zero_pos_, &QPushButton::clicked, this, &MainWindow::setZeroPositionButtonClicked);

    ui_timer_ = new QTimer(this);
    connect(ui_timer_, &QTimer::timeout, this, &MainWindow::updateUiState);
    ui_timer_->start(100);

    setWindowTitle("ROS2 + Qt Control Panel");
    resize(500, 320);
    updateUiState();
}

void MainWindow::setMotorIndicator(ButtonControl::MotorIndicatorState state)
{
    switch (state) {
        case ButtonControl::MotorIndicatorState::Yellow:
            label_motor_indicator_->setStyleSheet(kIndicatorYellow);
            break;
        case ButtonControl::MotorIndicatorState::Green:
            label_motor_indicator_->setStyleSheet(kIndicatorGreen);
            break;
        case ButtonControl::MotorIndicatorState::Red:
        default:
            label_motor_indicator_->setStyleSheet(kIndicatorRed);
            break;
    }
}

void MainWindow::updateUiState()
{
    if (!ros_node_) {
        return;
    }

    setMotorIndicator(ros_node_->motorIndicatorState());
    label_motor_indicator_->setToolTip(QString::fromStdString(ros_node_->motorIndicatorTooltip()));

    const bool has_feedback = ros_node_->hasFeedback();
    const bool motion = ros_node_->motionActive();
    // const bool any_enabled = ros_node_->anyMotorEnabled();
    // const bool all_enabled = ros_node_->motorsEnabled();
    // const bool all_disabled = ros_node_->motorsDisabled();

    button_start_motors_->setEnabled(has_feedback && !motion);
    button_stop_motors_->setEnabled(has_feedback && !motion);
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // button_start_motors_->setEnabled(has_feedback && !all_enabled && !motion);
    // button_stop_motors_->setEnabled(has_feedback && !all_disabled && !motion);

    button_set_zero_pos_->setEnabled(has_feedback && !motion && !ros_node_->hasMotorFault());
    // TODO: вернуть проверку any_enabled, когда enabled в топике стабилен:
    // button_set_zero_pos_->setEnabled(has_feedback && !any_enabled && !motion && !ros_node_->hasMotorFault());

    button_standing_->setEnabled(has_feedback && !motion);
    button_lying_down_->setEnabled(has_feedback && !motion);
    // TODO: вернуть, когда /low_level_state.enabled стабильно работает:
    // button_standing_->setEnabled(
    //     has_feedback && all_enabled && !motion &&
    //     !ros_node_->isAtCompletedPose(ButtonControl::PoseMotionKind::Standing));
    // button_lying_down_->setEnabled(
    //     has_feedback && all_enabled && !motion &&
    //     !ros_node_->isAtCompletedPose(ButtonControl::PoseMotionKind::Lying));
}

void MainWindow::lyingDownButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishLyingPose();
        updateUiState();
    }
}

void MainWindow::standingButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishStandingPose();
        updateUiState();
    }
}

void MainWindow::setZeroPositionButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishSetZeroCmdMessage();
        updateUiState();
    }
}

void MainWindow::startMotorsButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishStartMotorsCmdMessage();
        updateUiState();
    }
}

void MainWindow::stopMotorsButtonClicked()
{
    if (ros_node_) {
        ros_node_->publishStopMotorsCmdMessage();
        updateUiState();
    }
}

}  // namespace button_control
