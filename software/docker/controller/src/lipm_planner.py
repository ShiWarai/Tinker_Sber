import numpy as np

from scipy.spatial.transform import Rotation as R


def _wrap_to_pi(angle: float) -> float:
    return float((angle + np.pi) % (2 * np.pi) - np.pi)


def _wrap_arr(angles: np.ndarray) -> np.ndarray:
    return (angles + np.pi) % (2 * np.pi) - np.pi


class LIPMStepPlanner:
    # Minimum distance between step commands to avoid foot collision [m]
    FOOT_COLLISION_THRESHOLD = 0.15
    # Fraction of step radius used as swing-foot apex height (matches Isaac Gym cfg)
    APEX_HEIGHT_PERCENTAGE = 0.15

    def __init__(
        self,
        dt: float,
        step_period: int = 25,
        dstep_width: float = 0.24,
        dstep_length: float = 0.05,
        g: float = 9.81,
    ):

        self.dt           = dt
        self.step_period  = step_period
        self.full_period  = 2 * step_period
        self.dstep_width  = dstep_width
        self.dstep_length = dstep_length
        self.g            = g

        # Phase and timing state
        self.phase        = 0.0    # [0, 1)
        self.phase_count  = 0      # ticks since last phase reset
        self.update_count = 0      # ticks since last step command update
        self.step_stance  = step_period

        # foot_on_motion[0]=RIGHT, foot_on_motion[1]=LEFT
        # True means that foot is currently swinging (not in contact)
        self.foot_on_motion = np.array([False, True])  # left starts as swing

        # Step commands in world frame: shape (2, 3) = [right, left] x [x, y, heading]
        self.step_commands      = np.zeros((2, 3))
        self.prev_step_commands = np.zeros((2, 3))
        self.current_step       = np.zeros((2, 3))

        # LIPM state
        self.CoM = np.zeros(3)
        self.ICP = np.zeros(3)
        self.w   = 0.0

        # Base velocity estimate (updated externally each tick)
        self.base_vel_world  = np.zeros(3)
        self.base_pos_world  = np.zeros(3)

    def reset(self, init_stance_half_width: float = 0.054, init_heading: float = 0.0):
        """Reset planner to initial double-support stance."""
        self.phase        = 0.0
        self.phase_count  = 0
        self.update_count = 0
        self.step_stance  = self.step_period
        self.foot_on_motion = np.array([False, True])

        self.step_commands[0] = [0., -init_stance_half_width, init_heading]
        self.step_commands[1] = [0.,  init_stance_half_width, init_heading]
        self.prev_step_commands = self.step_commands.copy()
        self.current_step       = self.step_commands.copy()

        self.ICP[:]          = 0.
        self.base_vel_world  = np.zeros(3)
        self.base_pos_world  = np.zeros(3)

    # Main update
    def update(
        self,
        base_pos:  np.ndarray,
        base_vel:  np.ndarray,
        base_heading: float,
        commands:  np.ndarray,
        foot_pos_right_world: np.ndarray,
        foot_pos_left_world:  np.ndarray,
        foot_heading_right:   float,
        foot_heading_left:    float,
        com:       np.ndarray | None = None,
    ):

        self.base_pos_world = base_pos.copy()
        self.base_vel_world = base_vel.copy()

        # Phase update 
        self.phase_count  += 1
        self.update_count += 1
        self.phase += 1.0 / self.full_period
        if self.phase >= 1.0:
            self.phase -= 1.0

        # Reset phase each full period
        if self.phase_count >= self.full_period:
            self.phase_count = 0
            self.phase = 0.0

        # LIPM state
        com_est    = com if com is not None else base_pos
        
        CoM_height = max(float(com_est[2]), 0.05)
        # CoM_height = 0.28

        
        self.CoM   = com_est.copy()
        self.w     = float(np.sqrt(self.g / CoM_height))
        self.ICP[:2] = self.CoM[:2] + base_vel[:2] / self.w

        # Update current foot positions
        self.current_step[0] = [foot_pos_right_world[0], foot_pos_right_world[1], foot_heading_right]
        self.current_step[1] = [foot_pos_left_world[0],  foot_pos_left_world[1],  foot_heading_left]

        # Step command update
        if self.update_count >= self.step_period:
            self.prev_step_commands = self.step_commands.copy()
            self.update_count = 0
            self.step_stance  = self.step_period
            self.foot_on_motion = ~self.foot_on_motion  # alternate swing foot

            new_cmd = self._xcom_step_command(commands, base_pos, base_vel, base_heading)
            swing_idx = int(np.argmax(self.foot_on_motion))  # 0=right, 1=left
            self.step_commands[swing_idx] = new_cmd[swing_idx]

            # Collision avoidance
            dist = np.linalg.norm(
                self.step_commands[0, :2] - self.step_commands[1, :2])
            if dist < self.FOOT_COLLISION_THRESHOLD:
                self.step_commands = self._adjust_foot_collision(
                    self.step_commands)

    def get_phase_obs(self):

        p      = 2.0 * np.pi * self.phase
        sin_p  = np.sin(p)
        cos_p  = np.cos(p)
        eps    = 0.2
        return np.float32(sin_p), np.float32(cos_p)

    def get_step_commands_body(self, base_pos: np.ndarray, base_quat_xyzw: np.ndarray):

        rot_inv = R.from_quat(base_quat_xyzw).inv()

        base_heading = float(np.arctan2(
            2 * (base_quat_xyzw[3] * base_quat_xyzw[2]
                 + base_quat_xyzw[0] * base_quat_xyzw[1]),
            1 - 2 * (base_quat_xyzw[1] ** 2 + base_quat_xyzw[2] ** 2)))

        def _to_body(cmd_world):
            world_vec = np.array([cmd_world[0], cmd_world[1], 0.0]) - base_pos
            body_xyz  = rot_inv.apply(world_vec).astype(np.float32)
            head_rel  = np.float32(_wrap_to_pi(cmd_world[2] - base_heading))
            return np.array([*body_xyz, head_rel], dtype=np.float32)

        return _to_body(self.step_commands[0]), _to_body(self.step_commands[1])

    def get_foot_ref_trajectory(self, base_pos: np.ndarray, base_quat_xyzw: np.ndarray):

        rot_inv = R.from_quat(base_quat_xyzw).inv()

        def _ref_world(i: int) -> np.ndarray:
            prev_xy = self.prev_step_commands[i, :2]
            curr_xy = self.step_commands[i, :2]
            ref_xy  = prev_xy + (curr_xy - prev_xy) * self.phase
            center  = (prev_xy + curr_xy) * 0.5
            radius  = float(np.linalg.norm(curr_xy - prev_xy)) * 0.5
            apex    = self.APEX_HEIGHT_PERCENTAGE * radius
            if apex > 1e-6:
                a_sq    = radius ** 2 / apex
                dist_sq = float(np.sum((ref_xy - center) ** 2))
                z = max(0.0, -dist_sq / a_sq + apex)
            else:
                z = 0.0
            return np.array([ref_xy[0], ref_xy[1], z])

        def _to_body(pos_world: np.ndarray) -> np.ndarray:
            return rot_inv.apply(pos_world - base_pos).astype(np.float32)

        return _to_body(_ref_world(0)), _to_body(_ref_world(1))

    def _xcom_step_command(
        self,
        commands:     np.ndarray,
        base_pos:     np.ndarray,
        base_vel:     np.ndarray,
        base_heading: float,
    ) -> np.ndarray:

        T      = self.step_period * self.dt
        w      = self.w
        CoM    = self.CoM
        theta  = float(np.arctan2(commands[1], commands[0]))

        # Support foot position
        swing_idx   = int(np.argmax(self.foot_on_motion))
        support_idx = 1 - swing_idx
        support_pos = self.current_step[support_idx, :3].copy()
        # z from base height estimate (support foot on ground)
        support_pos_3d = np.array([support_pos[0], support_pos[1], 0.0])

        # LIPM forward integration over horizon T
        x0  = CoM[0] - support_pos_3d[0]
        y0  = CoM[1] - support_pos_3d[1]
        vx0 = base_vel[0]
        vy0 = base_vel[1]

        ch, sh = np.cosh(T * w), np.sinh(T * w)
        x_f  = x0 * ch + vx0 * sh / w
        vx_f = x0 * w * sh + vx0 * ch
        y_f  = y0 * ch + vy0 * sh / w
        vy_f = y0 * w * sh + vy0 * ch

        eICP_x = x_f + support_pos_3d[0] + vx_f / w
        eICP_y = y_f + support_pos_3d[1] + vy_f / w

        # XCoM offsets
        speed = float(np.linalg.norm(commands[:2]))
        dlen  = speed * T
        b_x   = dlen / (np.exp(T * w) - 1 + 1e-6)
        b_y   = self.dstep_width / (np.exp(T * w) + 1 + 1e-6)

        ox = -b_x
        oy_right = -b_y
        oy_left  =  b_y

        # Rotate offsets from heading-aligned to world frame
        c, s = np.cos(theta), np.sin(theta)

        def _rotate(ox_h, oy_h):
            return c * ox_h - s * oy_h, s * ox_h + c * oy_h

        ox_r, oy_r = _rotate(ox, oy_right)
        ox_l, oy_l = _rotate(ox, oy_left)

        new_cmds = self.step_commands.copy()
        if swing_idx == 0:   # right is swing
            new_cmds[0] = [eICP_x + ox_r, eICP_y + oy_r, theta]
        else:                 # left is swing
            new_cmds[1] = [eICP_x + ox_l, eICP_y + oy_l, theta]

        return new_cmds

    def _adjust_foot_collision(self, cmds: np.ndarray) -> np.ndarray:

        diff     = cmds[0, :2] - cmds[1, :2]
        dist     = float(np.linalg.norm(diff))
        if dist < 1e-6:
            return cmds
        swing_idx = int(np.argmax(self.foot_on_motion))
        out       = cmds.copy()
        out[swing_idx, :2] = (
            cmds[1 - swing_idx, :2]
            + self.FOOT_COLLISION_THRESHOLD * diff / dist)
        return out
