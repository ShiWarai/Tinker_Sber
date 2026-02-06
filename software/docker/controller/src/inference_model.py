import torch
import gymnasium as gym
# from modules.actor_critic import ActorCriticRMA, ActorCriticMixedBarlowTwins
from rsl_rl.runners import OnPolicyRunner


import argparse
parser = argparse.ArgumentParser(description="Train an RL agent with RSL-RL.")
parser.add_argument("--num_envs", type=int, default=1, help="Number of environments to simulate.")
parser.add_argument("--task", type=str, default='tk_blind_flat', help="Name of the task.")
parser.add_argument("--seed", type=int, default=None, help="Seed used for the environment")
parser.add_argument("--checkpoint_path", type=str, default=None, help="Relative path to checkpoint file.")

import cli_args
# append RSL-RL cli arguments
cli_args.add_rsl_rl_args(parser)
args_cli = parser.parse_args()

from .cfg import RslRlOnPolicyRunnerMlpCfg

class InferenceModel():
    def __init__(self, model_path):

        env = gym.make(id='tk_blind_flat')
        
        ppo_runner = OnPolicyRunner(env)

        agent_cfg: RslRlOnPolicyRunnerMlpCfg = cli_args.parse_rsl_rl_cfg(args_cli.task, args_cli)

        ppo_runner = OnPolicyRunner(
            env, agent_cfg.to_dict(), log_dir=None, device=agent_cfg.device
        )

        ppo_runner.load(model_path)

        self.policy = ppo_runner.get_inference_policy(device=env.unwrapped.device)

    def run(self, current_obs: torch.Tensor):

        '''self.obs_history = torch.cat([
            self.obs_history[1:],                    # [9, 39]
            current_obs.squeeze(0).unsqueeze(0)      # [1, 39]
        ], dim=0)                                    # [10, 39]
        
        obs_hist = self.obs_history.unsqueeze(0)     # [1, 10, 39]
        
        with torch.no_grad():
            output_tensor = self.model.actor_teacher_backbone(current_obs, obs_hist)

        # if full_obs.dim() == 2 and full_obs.shape[0] == 1:
        #     action = output_tensor.squeeze(0).cpu().numpy()
        # else:
        action = output_tensor.cpu().numpy()'''

        # From play.py:
        #obs_dict = env.get_observations()
        #obs = obs_dict["policy"]

        with torch.inference_mode():
            actions = self.policy(obs_dict)
        
        return actions