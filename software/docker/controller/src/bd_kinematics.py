import os
import numpy as np
import pinocchio as pin


class BDKinematics:
    
    FOOT_HEIGHT_OFFSET = -0.035

    def __init__(self, urdf_path: str | None = None):
        if urdf_path is None:
            urdf_path = os.path.join(os.path.dirname(__file__), 'model', 'BD.urdf')

        self.model = pin.buildModelFromUrdf(urdf_path)
        self.data = self.model.createData()

        self._right_ankle_id = self.model.getFrameId('R4_Link_ankle')
        self._left_ankle_id  = self.model.getFrameId('L4_Link_ankle')
        self._right_hip_id   = self.model.getFrameId('R0_Link')
        self._left_hip_id    = self.model.getFrameId('L0_Link')

        self._foot_offset = np.array([0., 0., self.FOOT_HEIGHT_OFFSET])
        self._forward     = np.array([1., 0., 0.])

    def compute(self, joint_pos: np.ndarray):

        q = np.asarray(joint_pos, dtype=float)
        pin.forwardKinematics(self.model, self.data, q)
        pin.updateFramePlacements(self.model, self.data)

        foot_right, heading_right = self._foot_state(self._right_ankle_id)
        foot_left,  heading_left  = self._foot_state(self._left_ankle_id)

        hip_right = self.data.oMf[self._right_hip_id].translation.copy()
        hip_left  = self.data.oMf[self._left_hip_id].translation.copy()

        return (
            np.array([*foot_right, heading_right], dtype=np.float32),
            np.array([*foot_left,  heading_left],  dtype=np.float32),
            hip_right.astype(np.float32),
            hip_left.astype(np.float32),
        )

    def compute_com(self, joint_pos: np.ndarray) -> np.ndarray:

        q = np.asarray(joint_pos, dtype=float)
        com = pin.centerOfMass(self.model, self.data, q)
        return np.array(com, dtype=np.float32)

    def _foot_state(self, frame_id: int):
        T = self.data.oMf[frame_id]
        pos = T.translation + T.rotation @ self._foot_offset
        fwd = T.rotation @ self._forward
        heading = float(np.arctan2(fwd[1], fwd[0]))
        return pos.copy(), heading
