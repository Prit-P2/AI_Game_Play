"""Train a PyTorch DQN against the game's headless C++ agent interface."""

from __future__ import annotations

import argparse
import random
import subprocess
from collections import deque
from pathlib import Path
from typing import TypeAlias

import torch
from torch import nn
from torch.nn import functional as F


OBSERVATION_SIZE = 327
ACTION_COUNT = 3
Transition: TypeAlias = tuple[list[float], int, float, list[float], float]


def find_game_executable() -> Path:
    root = Path(__file__).resolve().parent
    names = ("main.exe", "main")
    candidates = [
        root / "build" / "bin" / configuration / name
        for configuration in ("Release", "RelWithDebInfo", "Debug", "")
        for name in names
    ]
    binary_directory = root / "build" / "bin"
    if binary_directory.is_dir():
        candidates.extend(sorted(
            path for path in binary_directory.rglob("*")
            if path.is_file() and path.name in names
        ))

    existing = list(dict.fromkeys(path for path in candidates if path.is_file()))
    if not existing:
        raise FileNotFoundError(
            "Could not find the game executable. Build it with "
            "`cmake -S . -B build` and `cmake --build build --config Release`, "
            "or pass its path with --game."
        )

    source_files = (
        root / "src" / "main.cpp",
        root / "src" / "Game.cpp",
        root / "src" / "Game.h",
        root / "CMakeLists.txt",
    )
    newest_source = max(
        (path.stat().st_mtime for path in source_files if path.is_file()),
        default=0.0,
    )
    up_to_date = [
        path for path in existing if path.stat().st_mtime >= newest_source
    ]
    if up_to_date:
        return max(up_to_date, key=lambda path: path.stat().st_mtime)

    raise FileNotFoundError(
        "Found a game executable, but it is older than the game sources. "
        "Rebuild with `cmake --build build --config Release`, or pass a current "
        "executable with --game."
    )


class CppGameEnv:
    """Small Gymnasium-style adapter for the game's line-based C++ interface."""

    def __init__(
        self, executable: Path, frame_skip: int = 4, display: bool = False
    ) -> None:
        if frame_skip <= 0:
            raise ValueError("frame_skip must be positive")
        executable = executable.resolve()
        self.frame_skip = frame_skip
        self._process = subprocess.Popen(
            [str(executable), "--play" if display else "--rl"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
            cwd=executable.parent,
        )
        self._done = True

    @property
    def running(self) -> bool:
        return self._process.poll() is None

    def _request(self, command: str, expected_values: int) -> list[float]:
        if self._process.poll() is not None:
            raise RuntimeError(f"Game process exited with code {self._process.returncode}")
        if self._process.stdin is None or self._process.stdout is None:
            raise RuntimeError("Could not open the game process pipes")

        self._process.stdin.write(command + "\n")
        self._process.stdin.flush()
        line = self._process.stdout.readline()
        if not line:
            raise RuntimeError("Game process closed its output before replying")

        try:
            values = [float(value) for value in line.rstrip().split(",")]
        except ValueError as error:
            raise RuntimeError(
                "Game returned non-protocol output. Rebuild the current executable "
                f"and pass it with --game if needed. Output: {line!r}"
            ) from error
        if len(values) != expected_values:
            raise RuntimeError(
                f"Expected {expected_values} values from game, received {len(values)}"
            )
        return values

    def reset(self, seed: int | None = None) -> tuple[list[float], dict[str, object]]:
        if seed is None:
            seed = random.randrange(0, 2**32)
        observation = self._request(f"RESET {seed % (2**32)}", OBSERVATION_SIZE)
        self._done = False
        return observation, {}

    def step(
        self, action: int
    ) -> tuple[list[float], float, bool, bool, dict[str, object]]:
        if self._done:
            raise RuntimeError("Episode is over; call reset() before step()")
        if action not in (0, 1, 2):
            raise ValueError("action must be 0 (stay), 1 (left), or 2 (right)")

        values = self._request(
            f"STEP {action} {self.frame_skip}", OBSERVATION_SIZE + 2
        )
        reward = values[0]
        terminated = bool(values[1])
        self._done = terminated
        return values[2:], reward, terminated, False, {}

    def close(self) -> None:
        if self._process.poll() is None:
            if self._process.stdin is not None:
                try:
                    self._process.stdin.write("QUIT\n")
                    self._process.stdin.flush()
                except OSError:
                    pass
                finally:
                    try:
                        self._process.stdin.close()
                    except OSError:
                        pass
            self._process.wait(timeout=5)
        if self._process.stdout is not None:
            self._process.stdout.close()

    def __enter__(self) -> CppGameEnv:
        return self

    def __exit__(self, *_: object) -> None:
        self.close()


class QNetwork(nn.Module):
    def __init__(self) -> None:
        super().__init__()
        self.layers = nn.Sequential(
            nn.Linear(OBSERVATION_SIZE, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Linear(128, ACTION_COUNT),
        )

    def forward(self, observation: torch.Tensor) -> torch.Tensor:
        return self.layers(observation)


def select_device(selection: str, activity: str = "Training") -> torch.device:
    cuda_available = torch.cuda.is_available()
    if selection == "cuda" and not cuda_available:
        raise RuntimeError(
            "CUDA was requested but is unavailable. Install a CUDA-enabled "
            "PyTorch build and check that an NVIDIA GPU and driver are available."
        )

    if selection == "auto":
        device = torch.device("cuda" if cuda_available else "cpu")
    else:
        device = torch.device(selection)

    if device.type == "cuda":
        torch.cuda.init()
        print(f"{activity} device: {device} ({torch.cuda.get_device_name(device)})")
    else:
        print(f"{activity} device: {device}")
    return device


def train(args: argparse.Namespace) -> None:
    if args.episodes <= 0 or args.batch_size <= 0 or args.max_steps <= 0:
        raise ValueError("episodes, batch-size, and max-steps must be positive")

    random.seed(args.seed)
    torch.manual_seed(args.seed)
    torch.set_num_threads(1)
    device = select_device(args.device)
    if device.type == "cuda":
        torch.cuda.manual_seed_all(args.seed)

    online = QNetwork().to(device)
    target = QNetwork().to(device)
    target.load_state_dict(online.state_dict())
    target.eval()
    optimizer = torch.optim.Adam(online.parameters(), lr=args.learning_rate)
    replay: deque[Transition] = deque(maxlen=args.replay_size)
    total_steps = 0

    with CppGameEnv(args.game, args.frame_skip) as env:
        print(f"Game executable: {args.game}")
        for episode in range(1, args.episodes + 1):
            observation, _ = env.reset(args.seed + episode)
            episode_reward = 0.0
            episode_steps = 0
            terminated = False

            while not terminated and episode_steps < args.max_steps:
                epsilon = max(
                    args.epsilon_end,
                    1.0 - total_steps / args.epsilon_decay_steps
                    * (1.0 - args.epsilon_end),
                )
                if random.random() < epsilon:
                    action = random.randrange(ACTION_COUNT)
                else:
                    with torch.no_grad():
                        state = torch.tensor(
                            observation, dtype=torch.float32, device=device
                        ).unsqueeze(0)
                        action = int(online(state).argmax(dim=1).item())

                next_observation, reward, terminated, _, _ = env.step(action)
                episode_steps += 1
                truncated = episode_steps >= args.max_steps and not terminated
                replay.append(
                    (
                        observation,
                        action,
                        reward,
                        next_observation,
                        float(terminated or truncated),
                    )
                )
                observation = next_observation
                episode_reward += reward
                total_steps += 1

                if len(replay) >= args.batch_size:
                    batch = random.sample(replay, args.batch_size)
                    states = torch.tensor(
                        [item[0] for item in batch], dtype=torch.float32, device=device
                    )
                    actions = torch.tensor(
                        [item[1] for item in batch], dtype=torch.int64, device=device
                    ).unsqueeze(1)
                    rewards = torch.tensor(
                        [item[2] for item in batch], dtype=torch.float32, device=device
                    )
                    next_states = torch.tensor(
                        [item[3] for item in batch], dtype=torch.float32, device=device
                    )
                    dones = torch.tensor(
                        [item[4] for item in batch], dtype=torch.float32, device=device
                    )

                    current_q = online(states).gather(1, actions).squeeze(1)
                    with torch.no_grad():
                        next_q = target(next_states).max(dim=1).values
                        expected_q = rewards + args.gamma * (1.0 - dones) * next_q
                    loss = F.smooth_l1_loss(current_q, expected_q)
                    optimizer.zero_grad()
                    loss.backward()
                    nn.utils.clip_grad_norm_(online.parameters(), 10.0)
                    optimizer.step()

                if total_steps % args.target_update_steps == 0:
                    target.load_state_dict(online.state_dict())

            print(
                f"episode={episode}/{args.episodes} steps={episode_steps} "
                f"reward={episode_reward:.1f} epsilon={epsilon:.3f}"
            )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    torch.save(
        {
            "model_state_dict": online.state_dict(),
            "observation_size": OBSERVATION_SIZE,
            "action_count": ACTION_COUNT,
        },
        args.output,
    )
    print(f"Saved model to {args.output}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", type=Path)
    parser.add_argument("--episodes", type=int, default=500)
    parser.add_argument("--max-steps", type=int, default=2500)
    parser.add_argument("--frame-skip", type=int, default=4)
    parser.add_argument("--batch-size", type=int, default=64)
    parser.add_argument("--replay-size", type=int, default=50_000)
    parser.add_argument("--target-update-steps", type=int, default=500)
    parser.add_argument("--epsilon-decay-steps", type=int, default=50_000)
    parser.add_argument("--epsilon-end", type=float, default=0.05)
    parser.add_argument("--gamma", type=float, default=0.99)
    parser.add_argument("--learning-rate", type=float, default=1e-4)
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument(
        "--device",
        choices=("auto", "cpu", "cuda"),
        default="auto",
        help="Training device; auto selects CUDA when available",
    )
    parser.add_argument(
        "--output", type=Path, default=Path("models/space_shooter_dqn.pt")
    )
    args = parser.parse_args()
    if args.game is None:
        args.game = find_game_executable()
    args.game = args.game.resolve()
    if not args.game.is_file():
        parser.error(f"Game executable does not exist: {args.game}")
    return args


if __name__ == "__main__":
    train(parse_args())
