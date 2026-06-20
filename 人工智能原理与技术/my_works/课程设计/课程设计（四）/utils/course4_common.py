# -*- coding: utf-8 -*-
"""
课程设计四公共函数。
"""

from __future__ import annotations

import json
import platform
import random
from pathlib import Path
from typing import Any

import numpy as np
import torch


MODEL_ID = "stable-diffusion-v1-5/stable-diffusion-v1-5"
DEFAULT_SEED = 2452769


def get_project_root(current_file: str | Path) -> Path:
    """根据题目脚本位置获取项目根目录。"""
    return Path(current_file).resolve().parents[2]


def get_course_design_dir(current_file: str | Path) -> Path:
    """获取课程设计代码和输出所在目录。"""
    return get_project_root(current_file) / "my_works" / "课程设计"


def make_output_dir(current_file: str | Path, question_name: str) -> Path:
    """创建并返回指定小问的输出目录。"""
    output_dir = get_course_design_dir(current_file) / "course4_outputs" / question_name
    output_dir.mkdir(parents=True, exist_ok=True)
    return output_dir


def set_random_seed(seed: int, *, allow_tf32: bool = True) -> None:
    """固定随机种子，降低训练和采样结果波动。"""
    random.seed(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)
        torch.backends.cuda.matmul.allow_tf32 = allow_tf32
    torch.backends.cudnn.benchmark = False


def get_runtime_environment() -> dict[str, str | None]:
    """获取 Python、PyTorch 和 CUDA 运行环境信息。"""
    return {
        "python": platform.python_version(),
        "torch": torch.__version__,
        "cuda_runtime": torch.version.cuda,
        "gpu": torch.cuda.get_device_name(0) if torch.cuda.is_available() else None,
    }


def write_json(path: Path, data: dict[str, Any]) -> None:
    """以 UTF-8 编码写入 JSON 文件。"""
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, ensure_ascii=False, indent=2), encoding="utf-8")


def count_parameters(model: torch.nn.Module) -> tuple[int, int]:
    """统计模型总参数量和可训练参数量。"""
    total = sum(parameter.numel() for parameter in model.parameters())
    trainable = sum(parameter.numel() for parameter in model.parameters() if parameter.requires_grad)
    return total, trainable
