"""Run a saved PyTorch DQN in the visible SFML game."""

from __future__ import annotations

import argparse
from pathlib import Path

import torch

from rl_train import CppGameEnv, OBSERVATION_SIZE, QNetwork, find_game_executable
from rl_train import select_device


def parse_args() -> argparse.Namespace:
    project_root = Path(__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--model",
        type=Path,
        default=project_root / "models" / "space_shooter_dqn.pt",
    )
    parser.add_argument("--game", type=Path)
    parser.add_argument("--episodes", type=int, default=1)
    parser.add_argument("--frame-skip", type=int, default=1)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument(
        "--device",
        choices=("auto", "cpu", "cuda"),
        default="auto",
        help="Inference device; auto selects CUDA when available",
    )
    args = parser.parse_args()
    if args.game is None:
        args.game = find_game_executable()
    args.game = args.game.resolve()
    args.model = args.model.resolve()
    if not args.game.is_file():
        parser.error(f"Game executable does not exist: {args.game}")
    if not args.model.is_file():
        parser.error(f"Model file does not exist: {args.model}")
    if args.episodes <= 0 or args.frame_skip <= 0:
        parser.error("--episodes and --frame-skip must be positive")
    return args


def main() -> None:
    args = parse_args()
    device = select_device(args.device, "Playback")
    checkpoint = torch.load(args.model, map_location=device, weights_only=True)
    if checkpoint.get("observation_size") != OBSERVATION_SIZE:
        raise ValueError("Model observation size does not match this game")

    model = QNetwork().to(device)
    model.load_state_dict(checkpoint["model_state_dict"])
    model.eval()

    print(f"Model: {args.model}")
    print("Close the game window or press Escape to stop playback.")
    with CppGameEnv(args.game, frame_skip=args.frame_skip, display=True) as env:
        for episode in range(1, args.episodes + 1):
            observation, _ = env.reset(args.seed + episode - 1)
            episode_reward = 0.0
            terminated = False
            while not terminated:
                state = torch.tensor(
                    observation, dtype=torch.float32, device=device
                ).unsqueeze(0)
                with torch.inference_mode():
                    action = int(model(state).argmax(dim=1).item())
                observation, reward, terminated, _, _ = env.step(action)
                episode_reward += reward
            print(f"episode={episode}/{args.episodes} reward={episode_reward:.1f}")
            if not env.running:
                break


if __name__ == "__main__":
    main()
