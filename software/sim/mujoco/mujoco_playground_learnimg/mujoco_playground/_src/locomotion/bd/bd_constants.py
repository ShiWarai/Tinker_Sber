"""Constants for BD biped."""

from etils import epath

from mujoco_playground._src import mjx_env

ROOT_PATH = mjx_env.ROOT_PATH / "locomotion" / "bd"
FEET_ONLY_FLAT_TERRAIN_XML = (
    ROOT_PATH / "xmls" / "scene_mjx_feetonly_flat_terrain.xml"
)


def task_to_xml(task_name: str) -> epath.Path:
  return {
      "flat_terrain": FEET_ONLY_FLAT_TERRAIN_XML,
  }[task_name]


FEET_SITES = [
    "left_foot",
    "right_foot",
]

# Sole boxes (physics). Gait sensors use toe/heel spheres instead.
LEFT_FEET_GEOMS = [
    "left_foot",
]

RIGHT_FEET_GEOMS = [
    "right_foot",
]

FEET_GEOMS = LEFT_FEET_GEOMS + RIGHT_FEET_GEOMS

FEET_TOE_GEOMS = [
    "left_foot_toe",
    "right_foot_toe",
]

FEET_HEEL_GEOMS = [
    "left_foot_heel",
    "right_foot_heel",
]

FEET_POS_SENSOR = [f"{site}_pos" for site in FEET_SITES]

ROOT_BODY = "base_link"

GRAVITY_SENSOR = "upvector"
GLOBAL_LINVEL_SENSOR = "global_linvel"
GLOBAL_ANGVEL_SENSOR = "global_angvel"
LOCAL_LINVEL_SENSOR = "local_linvel"
ACCELEROMETER_SENSOR = "accelerometer"
GYRO_SENSOR = "gyro"
