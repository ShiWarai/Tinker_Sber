# SPDX-FileCopyrightText: Copyright (c) 2025
# SPDX-License-Identifier: BSD-3-Clause

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.distributions import Normal
from typing import Optional, Tuple, Dict

from omni.isaac.lab.utils.assets import read_file


class MixedMlp(nn.Module):
    """Mixed MLP with multiple experts (из common_modules)"""
    
    def __init__(self, input_size: int, latent_size: int, hidden_size: int, 
                 num_actions: int, num_experts: int = 4):
        super().__init__()
        
        self.num_experts = num_experts
        
        # Experts
        self.experts = nn.ModuleList([
            nn.Sequential(
                nn.Linear(latent_size, hidden_size),
                nn.ELU(),
                nn.Linear(hidden_size, hidden_size),
                nn.ELU(),
            ) for _ in range(num_experts)
        ])
        
        # Gating network
        self.gate = nn.Sequential(
            nn.Linear(input_size, hidden_size),
            nn.ELU(),
            nn.Linear(hidden_size, num_experts),
            nn.Softmax(dim=-1)
        )
        
        # Final layers
        self.final = nn.Sequential(
            nn.Linear(hidden_size + input_size, hidden_size),
            nn.ELU(),
            nn.Linear(hidden_size, num_actions)
        )
    
    def forward(self, latent: torch.Tensor, obs: torch.Tensor) -> torch.Tensor:
        # Get gating weights
        gate_weights = self.gate(obs)  # [batch, num_experts]
        
        # Apply experts
        expert_outputs = []
        for i, expert in enumerate(self.experts):
            expert_out = expert(latent)
            expert_outputs.append(expert_out.unsqueeze(1))
        
        expert_outputs = torch.cat(expert_outputs, dim=1)  # [batch, num_experts, hidden]
        
        # Weighted sum of expert outputs
        weighted = torch.sum(gate_weights.unsqueeze(-1) * expert_outputs, dim=1)
        
        # Combine with observation
        combined = torch.cat([weighted, obs], dim=-1)
        
        return self.final(combined)


class StateHistoryEncoder(nn.Module):
    """State History Encoder (из common_modules)"""
    
    def __init__(self, activation_fn, input_size: int, history_len: int, output_size: int):
        super().__init__()
        
        self.history_len = history_len
        self.input_size = input_size
        
        self.encoder = nn.Sequential(
            nn.Linear(input_size * history_len, 256),
            activation_fn(),
            nn.Linear(256, 128),
            activation_fn(),
            nn.Linear(128, output_size)
        )
    
    def forward(self, history: torch.Tensor) -> torch.Tensor:
        # history: [batch, history_len, input_size]
        batch_size = history.shape[0]
        flattened = history.reshape(batch_size, -1)
        return self.encoder(flattened)


class MixedMlpBarlowTwinsActor(nn.Module):
    """Mixed MLP Barlow Twins Actor (из actor_critic.py)"""
    
    def __init__(self,
                 num_prop: int,
                 num_hist: int,
                 num_actions: int,
                 mlp_encoder_dims: list,
                 actor_dims: list,
                 latent_dim: int = 16,
                 obs_encoder_dims: list = None,
                 activation: str = 'elu'):
        super().__init__()
        
        self.num_prop = num_prop
        self.num_hist = num_hist
        self.num_actions = num_actions
        self.latent_dim = latent_dim
        
        # Activation function
        act_fn = self._get_activation(activation)
        
        # MLP encoder for history
        encoder_layers = []
        prev_dim = num_prop * num_hist
        for dim in mlp_encoder_dims:
            encoder_layers.extend([
                nn.Linear(prev_dim, dim),
                nn.LayerNorm(dim),
                act_fn()
            ])
            prev_dim = dim
        encoder_layers.append(nn.Linear(prev_dim, latent_dim + 7))  # +7 for priv latent
        self.mlp_encoder = nn.Sequential(*encoder_layers)
        
        # Mixed MLP actor
        self.actor = MixedMlp(
            input_size=num_prop,
            latent_size=latent_dim + 7,
            hidden_size=128,
            num_actions=num_actions,
            num_experts=4
        )
        
        # Obs encoder for Barlow Twins
        obs_layers = []
        prev_dim = num_prop
        for dim in obs_encoder_dims:
            obs_layers.extend([
                nn.Linear(prev_dim, dim),
                nn.LayerNorm(dim),
                act_fn()
            ])
            prev_dim = dim
        obs_layers.append(nn.Linear(prev_dim, latent_dim))
        self.obs_encoder = nn.Sequential(*obs_layers)
        
        # Batch norm for Barlow Twins
        self.bn = nn.BatchNorm1d(latent_dim, affine=False)
    
    def forward(self, obs: torch.Tensor, obs_hist: torch.Tensor) -> torch.Tensor:
        # Prepare full history (including current obs)
        obs_hist_full = torch.cat([
            obs_hist[:, 1:],
            obs.unsqueeze(1)
        ], dim=1)
        
        # Encode history
        batch_size = obs_hist_full.shape[0]
        latents = self.mlp_encoder(obs_hist_full.reshape(batch_size, -1))
        
        # Get action
        mean = self.actor(latents, obs)
        
        return mean
    
    def barlow_twins_loss(self, obs: torch.Tensor, obs_hist: torch.Tensor, 
                         priv: torch.Tensor, weight: float = 5e-3) -> torch.Tensor:
        """Barlow Twins loss for self-supervised learning"""
        batch_size = obs.shape[0]
        
        # Get predictions from history
        obs_hist_full = torch.cat([
            obs_hist[:, 1:],
            obs.unsqueeze(1)
        ], dim=1)
        
        predicted = self.mlp_encoder(obs_hist_full.reshape(batch_size, -1))
        hist_latent = predicted[:, 7:]  # Latent part
        priv_latent = predicted[:, :7]  # Priv part
        
        # Encode current observation
        obs_latent = self.obs_encoder(obs)
        
        # Barlow Twins loss
        c = self.bn(hist_latent).T @ self.bn(obs_latent)
        c.div_(batch_size)
        
        on_diag = torch.diagonal(c).add_(-1).pow_(2).sum()
        off_diag = self._off_diagonal(c).pow_(2).sum()
        
        # Priv loss
        priv_loss = F.mse_loss(priv_latent, priv)
        
        return on_diag + weight * off_diag + 0.01 * priv_loss
    
    def _off_diagonal(self, x: torch.Tensor) -> torch.Tensor:
        n, m = x.shape
        assert n == m
        return x.flatten()[:-1].view(n - 1, n + 1)[:, 1:].flatten()
    
    def _get_activation(self, name: str):
        if name == 'elu':
            return nn.ELU
        elif name == 'relu':
            return nn.ReLU
        elif name == 'tanh':
            return nn.Tanh
        else:
            raise ValueError(f"Unknown activation: {name}")


class ActorCriticRMA(nn.Module):
    """RMA Actor-Critic (из actor_critic.py)"""
    
    is_recurrent = False
    
    def __init__(self,
                 num_prop: int,
                 num_scan: int,
                 num_priv_latent: int,
                 num_hist: int,
                 num_actions: int,
                 num_costs: int = 5,
                 scan_encoder_dims: list = None,
                 actor_hidden_dims: list = [512, 256, 128],
                 critic_hidden_dims: list = [512, 256, 128],
                 priv_encoder_dims: list = [],
                 activation: str = 'elu',
                 init_noise_std: float = 1.0,
                 teacher_act: bool = False,
                 imi_flag: bool = False,
                 **kwargs):
        super().__init__()
        
        self.num_prop = num_prop
        self.num_scan = num_scan
        self.num_priv_latent = num_priv_latent
        self.num_hist = num_hist
        self.num_actions = num_actions
        self.num_costs = num_costs
        self.teacher_act = teacher_act
        self.imi_flag = imi_flag
        
        act_fn = self._get_activation(activation)
        
        # Private latent encoder
        if len(priv_encoder_dims) > 0:
            priv_layers = self._mlp_factory(
                act_fn, num_priv_latent, None, priv_encoder_dims, last_act=True
            )
            self.priv_encoder = nn.Sequential(*priv_layers)
            priv_encoder_output_dim = priv_encoder_dims[-1]
        else:
            self.priv_encoder = nn.Identity()
            priv_encoder_output_dim = num_priv_latent
        
        # Scan encoder
        if scan_encoder_dims is not None and num_scan > 0:
            scan_layers = self._mlp_factory(
                act_fn, num_scan, None, scan_encoder_dims, last_act=True
            )
            self.scan_encoder = nn.Sequential(*scan_layers)
            self.scan_encoder_output_dim = scan_encoder_dims[-1]
        else:
            self.scan_encoder = nn.Identity()
            self.scan_encoder_output_dim = num_scan
        
        # History encoder
        self.history_encoder = StateHistoryEncoder(
            act_fn, num_prop, num_hist, 32
        )
        
        # Teacher actor (with priv info)
        teacher_actor_layers = self._mlp_factory(
            act_fn,
            num_prop + priv_encoder_output_dim + 32,
            num_actions,
            actor_hidden_dims,
            last_act=False
        )
        self.actor_teacher_backbone = nn.Sequential(*teacher_actor_layers)
        
        # Student actor (Barlow Twins based)
        self.actor_student_backbone = MixedMlpBarlowTwinsActor(
            num_prop=num_prop,
            num_hist=num_hist,
            num_actions=num_actions,
            mlp_encoder_dims=[512, 256, 128],
            actor_dims=actor_hidden_dims,
            latent_dim=16,
            obs_encoder_dims=[256, 128],
            activation=activation
        )
        
        # Critic
        critic_input_dim = num_prop + self.scan_encoder_output_dim + priv_encoder_output_dim + 32
        critic_layers = self._mlp_factory(
            act_fn, critic_input_dim, 1, critic_hidden_dims, last_act=False
        )
        self.critic = nn.Sequential(*critic_layers)
        
        # Cost predictor (for NP3O)
        cost_layers = self._mlp_factory(
            act_fn, critic_input_dim, num_costs, critic_hidden_dims, last_act=False
        )
        cost_layers.append(nn.Softplus())
        self.cost = nn.Sequential(*cost_layers)
        
        # Action noise
        self.std = nn.Parameter(init_noise_std * torch.ones(num_actions))
        self.distribution = None
        
        # Disable args validation for speedup
        Normal.set_default_validate_args = False
    
    def set_teacher_act(self, flag: bool):
        self.teacher_act = flag
    
    def get_std(self):
        return self.std
    
    @property
    def action_mean(self):
        return self.distribution.mean
    
    @property
    def action_std(self):
        return self.distribution.stddev
    
    @property
    def entropy(self):
        return self.distribution.entropy().sum(dim=-1)
    
    def update_distribution(self, obs: torch.Tensor):
        if self.teacher_act:
            mean = self.act_teacher(obs)
        else:
            mean = self.act_student(obs)
        self.distribution = Normal(mean, mean * 0. + self.get_std())
    
    def act(self, obs: torch.Tensor, **kwargs) -> torch.Tensor:
        self.update_distribution(obs)
        return self.distribution.sample()
    
    def get_actions_log_prob(self, actions: torch.Tensor) -> torch.Tensor:
        return self.distribution.log_prob(actions).sum(dim=-1)
    
    def act_student(self, obs: torch.Tensor, **kwargs) -> torch.Tensor:
        obs_prop = obs[:, :self.num_prop]
        hist = obs[:, -self.num_hist * self.num_prop:].view(-1, self.num_hist, self.num_prop)
        return self.actor_student_backbone(obs_prop, hist)
    
    def act_teacher(self, obs: torch.Tensor, **kwargs) -> torch.Tensor:
        obs_prop = obs[:, :self.num_prop]
        latent = self.infer_priv_latent(obs)
        hist_latent = self.infer_hist_latent(obs)
        
        backbone_input = torch.cat([obs_prop, latent, hist_latent], dim=1)
        return self.actor_teacher_backbone(backbone_input)
    
    def evaluate(self, obs: torch.Tensor, **kwargs) -> torch.Tensor:
        obs_prop = obs[:, :self.num_prop]
        scan_latent = self.infer_scandots_latent(obs)
        latent = self.infer_priv_latent(obs)
        hist_latent = self.infer_hist_latent(obs)
        
        backbone_input = torch.cat([obs_prop, latent, scan_latent, hist_latent], dim=1)
        return self.critic(backbone_input)
    
    def evaluate_cost(self, obs: torch.Tensor, **kwargs) -> torch.Tensor:
        obs_prop = obs[:, :self.num_prop]
        scan_latent = self.infer_scandots_latent(obs)
        latent = self.infer_priv_latent(obs)
        hist_latent = self.infer_hist_latent(obs)
        
        backbone_input = torch.cat([obs_prop, latent, scan_latent, hist_latent], dim=1)
        return self.cost(backbone_input)
    
    def infer_priv_latent(self, obs: torch.Tensor) -> torch.Tensor:
        priv = obs[:, self.num_prop + self.num_scan:self.num_prop + self.num_scan + self.num_priv_latent]
        return self.priv_encoder(priv)
    
    def infer_scandots_latent(self, obs: torch.Tensor) -> torch.Tensor:
        scan = obs[:, self.num_prop:self.num_prop + self.num_scan]
        return self.scan_encoder(scan)
    
    def infer_hist_latent(self, obs: torch.Tensor) -> torch.Tensor:
        hist = obs[:, -self.num_hist * self.num_prop:]
        return self.history_encoder(hist.view(-1, self.num_hist, self.num_prop))
    
    def imitation_learning_loss(self, obs: torch.Tensor) -> torch.Tensor:
        with torch.no_grad():
            target_mean = self.act_teacher(obs)
        mean = self.act_student(obs)
        return F.mse_loss(mean, target_mean.detach())
    
    def barlow_twins_loss(self, obs: torch.Tensor, weight: float = 5e-3) -> torch.Tensor:
        obs_prop = obs[:, :self.num_prop]
        priv = obs[:, self.num_prop + self.num_scan:self.num_prop + self.num_scan + 7]
        obs_hist = obs[:, -self.num_hist * self.num_prop:].view(-1, self.num_hist, self.num_prop)
        
        return self.actor_student_backbone.barlow_twins_loss(obs_prop, obs_hist, priv, weight)
    
    def imitation_mode(self):
        self.actor_teacher_backbone.eval()
        self.scan_encoder.eval()
        self.priv_encoder.eval()
    
    def save_torch_jit_policy(self, path: str, device: torch.device):
        self.actor_student_backbone.eval()
        
        obs_demo = torch.randn(1, self.num_prop, device=device).half()
        hist_demo = torch.randn(1, self.num_hist, self.num_prop, device=device).half()
        
        traced = torch.jit.trace(self.actor_student_backbone, (obs_demo, hist_demo))
        traced.save(path)
    
    def _mlp_factory(self, activation, input_dims, out_dims, hidden_dims, last_act=False):
        layers = []
        prev_dim = input_dims
        
        for dim in hidden_dims:
            layers.append(nn.Linear(prev_dim, dim))
            layers.append(activation())
            prev_dim = dim
        
        if out_dims is not None:
            layers.append(nn.Linear(prev_dim, out_dims))
            if last_act:
                layers.append(activation())
        
        return layers
    
    def _get_activation(self, name: str):
        if name == 'elu':
            return nn.ELU
        elif name == 'relu':
            return nn.ReLU
        elif name == 'tanh':
            return nn.Tanh
        else:
            raise ValueError(f"Unknown activation: {name}")