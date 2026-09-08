#!/usr/bin/env python3

import argparse
from pathlib import Path

import rclpy
from rclpy.executors import MultiThreadedExecutor

from src.gait_controller import GaitController

DEFAULT_MODEL_DIR = Path(__file__).resolve().parent / "src" / "model" / "tinker"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-d", "--device",
                    type=str,
                    default="keyboard",
                    choices=['keyboard', 'gamepad'],
                    help="set input device: 'keyboard'/'gamepad' (default is keyboard)")
    ap.add_argument("-p", "--path",
                    type=str,
                    default=str(DEFAULT_MODEL_DIR),
                    help="path to ONNX policy directory (params.yaml + policy/policy.onnx)")
    args = ap.parse_args()

    rclpy.init()

    try:
        controller = GaitController(
            # adapter_type = args.object,
            device_type = args.device,
            model_path = args.path
        )

        executor = MultiThreadedExecutor()
        executor.add_node(controller)

        executor.spin()

    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f'Error: {e}')

    finally:
        if 'controller' in locals():
            controller.shutdown()
        rclpy.shutdown()

if __name__ == '__main__':
    main()