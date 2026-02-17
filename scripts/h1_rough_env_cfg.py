# SPDX-FileCopyrightText: Copyright (c) 2025
# SPDX-License-Identifier: BSD-3-Clause

from dataclasses import MISSING
from typing import Tuple, List, Dict, Optional

import omni.isaac.lab.sim as sim_utils
from omni.isaac.lab.assets import ArticulationCfg, AssetBaseCfg
from omni.isaac.lab.envs import DirectRLEnvCfg
from omni.isaac.lab.managers import SceneEntityCfg, RewardTermCfg as RewTerm
from omni.isaac.lab.managers import TerminationTermCfg as DoneTerm
from omni.isaac.lab.scene import InteractiveSceneCfg
from omni.isaac.lab.sensors import ContactSensorCfg, RayCasterCfg, patterns
from omni.isaac.lab.sim import SimulationCfg, PhysxCfg
from omni.isaac.lab.terrains import TerrainImporterCfg
from omni.isaac.lab.utils import configclass
from omni.isaac.lab.utils.noise import AdditiveUniformNoiseCfg as Unoise
from omni.isaac.lab.utils.assets import ISAAC_NUCLEUS_DIR


@configclass
class H1RoughEnvCfg(DirectRLEnvCfg):
    """Конфигурация для H1 на сложном рельефе с RMA архитектурой"""
    
    # Основные параметры
    episode_length_s = 20.0
    decimation = 4  # Политика выполняется каждый 4-й шаг (50 Hz при 200 Hz симуляции)
    num_actions = 10  # H1 имеет 10 actuated DOF
    
    # Размерности наблюдений (из h1_constraint_him_trot.py)
    n_proprio = 39
    n_scan = 187
    n_priv_latent = 47  # Рассчитано: 4+1+12+12+12+6+1+4+1-3+4-10
    history_len = 10
    
    num_observations = n_proprio + n_priv_latent + n_scan + history_len * n_proprio
    num_states = 0
    
    # Симуляция
    sim: SimulationCfg = SimulationCfg(
        dt=1 / 200,  # 200 Hz симуляция
        render_interval=decimation,
        physics_material=sim_utils.RigidBodyMaterialCfg(
            friction_combine_mode="multiply",
            restitution_combine_mode="multiply",
        ),
        physx=PhysxCfg(
            num_position_iterations=4,
            num_velocity_iterations=0,
            contact_offset=0.02,
            rest_offset=0.0,
            bounce_threshold_velocity=0.2,
            max_depenetration_velocity=10.0,
            gpu_max_rigid_contact_count=2**23,
        ),
    )
    
    # Сцена
    scene: InteractiveSceneCfg = InteractiveSceneCfg(
        num_envs=1024,  # Из конфига
        env_spacing=5.0,
        replicate_physics=True,
    )
    
    # Террейн (из LeggedRobot)
    terrain = TerrainImporterCfg(
        prim_path="/World/ground",
        terrain_type="plane",  # По умолчанию plane, можно изменить
        terrain_generator=None,
        physics_material=sim_utils.RigidBodyMaterialCfg(
            static_friction=1.0,
            dynamic_friction=1.0,
        ),
        visual_material=sim_utils.MdlFileCfg(
            mdl_path=f"{ISAAC_NUCLEUS_DIR}/Materials/Base/Architecture/Shingles_01.mdl",
            project_uvw=True,
        ),
        debug_vis=False,
    )
    
    # Робот H1 (из asset конфига)
    robot: ArticulationCfg = ArticulationCfg(
        spawn=sim_utils.UrdfFileCfg(
            asset_path="{ROOT_DIR}/resources/h1/urdf/h1.urdf",
            rigid_props=sim_utils.RigidBodyPropertiesCfg(
                disable_gravity=False,
                max_linear_velocity=1000.0,
                max_angular_velocity=1000.0,
                max_depenetration_velocity=10.0,
            ),
            articulation_props=sim_utils.ArticulationRootPropertiesCfg(
                enabled_self_collisions=True,  # self_collisions = 1 (disable) -> здесь наоборот
                solver_position_iteration_count=8,
                solver_velocity_iteration_count=0,
            ),
        ),
        init_state=ArticulationCfg.InitialStateCfg(
            pos=(0.0, 0.0, 0.33),
            joint_pos={
                "J_L0": 0.0,
                "J_L1": 0.08,
                "J_L2": 0.56,
                "J_L3": -1.12,
                "J_L4_ankle": -0.57,
                "J_R0": 0.0,
                "J_R1": -0.08,
                "J_R2": -0.56,
                "J_R3": 1.12,
                "J_R4_ankle": 0.57,
            },
        ),
        actuators={
            "legs": sim_utils.DCMotorCfg(
                joint_names_expr=[".*"],
                effort_limit=100.0,  # Из torque_limits
                saturation_effort=100.0,
                velocity_limit=10.0,  # Из dof_vel_limits
                stiffness={
                    "J_L0": 13.0,
                    "J_L1": 15.0,
                    "J_L2": 15.0,
                    "J_L3": 15.0,
                    "J_L4_ankle": 13.0,
                    "J_R0": 13.0,
                    "J_R1": 15.0,
                    "J_R2": 15.0,
                    "J_R3": 15.0,
                    "J_R4_ankle": 13.0,
                },
                damping={
                    "J_L0": 0.3,
                    "J_L1": 0.65,
                    "J_L2": 0.65,
                    "J_L3": 0.65,
                    "J_L4_ankle": 0.3,
                    "J_R0": 0.3,
                    "J_R1": 0.65,
                    "J_R2": 0.65,
                    "J_R3": 0.65,
                    "J_R4_ankle": 0.3,
                },
                armature=0.01,
            ),
        },
    )
    
    # Сенсоры
    contact_sensor: ContactSensorCfg = ContactSensorCfg(
        prim_path="/World/envs/env_.*/Robot/.*",
        update_period=sim.dt,
        history_length=3,
        debug_vis=False,
        filter_prim_paths_expr=["/World/envs/env_.*/Robot/.*/base", "/World/ground"],
    )
    
    # RayCaster для сканирования (аналог depth камеры, но проще)
    ray_caster: RayCasterCfg = RayCasterCfg(
        prim_path="/World/envs/env_.*/Robot/base",
        update_period=0.05,
        offset=RayCasterCfg.OffsetCfg(
            pos=(0.27, 0.0, 0.03),  # Из depth.position
            rot=(0.0, 0.0, 0.0, 1.0),
        ),
        patterns={
            "grid": patterns.PatternCfg(
                angles=(-0.5, 0.5, 0.5, -0.5),
                # 187 точек сканирования
            )
        },
        attach_yaw_only=True,
        debug_vis=False,
        mesh_prim_paths=["/World/ground"],
    )
    
    # Настройки шума (из noise конфига)
    noise_scales = {
        "dof_pos": 0.03,
        "dof_vel": 1.5,
        "lin_vel": 0.15,
        "ang_vel": 0.2,
        "gravity": 0.07,
        "quat": 0.05,
        "height_measurements": 0.02,
        "contact_states": 0.05,
    }
    
    observation_noise_model = Unoise(
        noise_bounds=[-0.05, 0.05],
        distribution="uniform",
        operation="add",
        sensor_cfg=None,
    )
    
    # Команды (из commands.ranges)
    @configclass
    class CommandSettings:
        lin_vel_x = [-0.5, 0.5]
        lin_vel_y = [-0.5, 0.5]
        ang_vel_yaw = [-1.5, 1.5]
        heading = [-3.14, 3.14]
        height = [0.12, 0.2]
        
        num_commands = 5  # [v_x, v_y, omega, heading, height]
        resampling_time = 12.0
        heading_command = True
        global_reference = False
        
    commands: CommandSettings = CommandSettings()
    
    # Доменная рандомизация
    @configclass
    class DomainRandSettings:
        # Физические свойства
        randomize_friction = True
        friction_range = [0.2, 2.75]
        randomize_restitution = True
        restitution_range = [0.0, 1.0]
        
        # Масса и COM
        randomize_base_mass = True
        added_mass_range = [-0.5, 0.5]
        randomize_base_com = True
        added_com_range = [-0.05, 0.05]
        
        # Рандомизация всех link-ов
        randomize_all_mass = True
        rd_mass_range = [0.9, 1.1]
        randomize_com = True
        rd_com_range = [-0.02, 0.02]
        random_inertia = True
        inertia_range = [0.9, 1.1]
        
        # Моторы
        randomize_motor = True
        motor_strength_range = [0.8, 1.2]
        randomize_motor_offset = True
        motor_offset_range = [-0.045, 0.045]
        
        # PD gains
        randomize_kpkd = True
        kp_range = [0.8, 1.2]
        kd_range = [0.8, 1.2]
        
        # Задержки (lags)
        randomize_lag_timesteps = True
        lag_timesteps = 8
        add_imu_lag = True
        randomize_imu_lag_timesteps = True
        imu_lag_timesteps_range = [0, 1]
        
        # Толчки
        push_robots = True
        push_interval_s = 6.0
        max_push_vel_xy = 0.7
        max_push_ang_vel = 0.6
        
        # Шум действий
        action_noise = 0.015
        
    domain_rand: DomainRandSettings = DomainRandSettings()
    
    # Настройки управления
    @configclass
    class ControlSettings:
        control_type = 'P'
        action_scale = 0.25
        decimation = 4
        hip_scale_reduction = 1.0
        use_filter = True
        
    control: ControlSettings = ControlSettings()
    
    # Награды (из rewards.scales)
    @configclass
    class RewardSettings:
        tracking_lin_vel = 2.5
        tracking_ang_vel = 2.0
        base_acc = 0.02
        lin_vel_z = -2.0
        ang_vel_xy = -0.05
        base_height = 0.2
        powers = -2e-5
        action_smoothness = -0.01
        torques = -1e-5
        dof_vel = -5e-4
        dof_acc = -2e-7
        stand_still_force = -0.1
        stand_2leg = 6.0
        feet_air_time = 3.0
        foot_clearance = -3.0
        stumble = -0.02
        no_jump = 0.7
        orientation_eular = 1.5
        hip_pos = -1.0
        feet_rotation1 = 0.3
        feet_rotation2 = 0.3
        feet_contact_forces = -0.01
        low_speed = 0.2
        track_vel_hard = 0.5
        foot_slip = -0.05
        
        termination = -5.0
        
    rewards: RewardSettings = RewardSettings()
    
    # Cost функции (из costs.scales)
    @configclass
    class CostSettings:
        pos_limit = 0.1
        torque_limit = 0.1
        dof_vel_limits = 0.1
        feet_air_time = 0.1
        hip_pos = 0.1
        
        num_costs = 5  # Для NP3O
        
    costs: CostSettings = CostSettings()
    
    # Параметры наград (дополнительные)
    @configclass
    class RewardParams:
        soft_dof_pos_limit = 0.9
        base_height_target = 0.30
        clearance_height_target = -0.21
        tracking_sigma = 0.5
        cycle_time = 0.5
        touch_thr = 6.0
        command_dead = 0.005
        stop_rate = 0.25
        target_joint_pos_scale = 0.17
        max_contact_force = 120.0
        
    reward_params: RewardParams = RewardParams()