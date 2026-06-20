# -*- coding: utf-8 -*-
"""
课程设计四 第五小问：换用 SDXL Base 1.0 并通过水墨风提示词生成对比图。
"""

from __future__ import annotations

import os
import sys
from dataclasses import dataclass, field
from pathlib import Path


SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from utils.course4_common import (
    DEFAULT_SEED,
    make_output_dir,
    set_random_seed,
)
from utils.course4_sdxl import (
    DEFAULT_SDXL_MODEL_PATH,
    run_sdxl_seed_generation,
    validate_sdxl_generation_config,
)


@dataclass(frozen=True)
class GenerationConfig:
    """第五小问水墨风提示词的模型、推理参数和保存设置。"""

    model_id: str = field(
        default_factory=lambda: os.environ.get("COURSE4_Q5_MODEL_PATH", DEFAULT_SDXL_MODEL_PATH)
    )
    output_dir: Path = field(default_factory=Path)
    seed: int = DEFAULT_SEED
    seeds: tuple[int, ...] = (2452769, 2452770, 2452771, 2452772)
    width: int = 1216
    height: int = 832
    num_inference_steps: int = 36
    guidance_scale: float = 7.8
    local_files_only: bool = True
    enable_cpu_offload: bool = True
    grid_columns: int = 2
    grid_thumbnail_width: int = 608
    prompt: str = (
        "Poem: 孤舟蓑笠翁，独钓寒江雪. "
        "Chinese ink wash painting, shanshui landscape, lone elderly fisherman, "
        "bamboo hat, straw rain cape, small boat on snowy river, "
        "misty mountains, xuan paper, black ink, empty space"
    )
    negative_prompt: str = (
        "photorealistic, western oil painting, colorful, modern city, crowd, "
        "motorboat, anime, cartoon, blurry, low quality, bad hands, text, watermark"
    )


def build_config() -> GenerationConfig:
    """构造第五小问水墨风运行配置。"""
    output_dir = make_output_dir(__file__, "question5")
    env_output_dir = os.environ.get("COURSE4_Q5_OUTPUT_DIR")
    if env_output_dir:
        output_dir = Path(env_output_dir)
    return GenerationConfig(output_dir=output_dir)


def validate_config(config: GenerationConfig) -> None:
    """检查 SDXL 水墨风推理参数。"""
    validate_sdxl_generation_config(config)


def main() -> None:
    """执行 SDXL 水墨风多 seed 推理并生成对比图。"""
    config = build_config()
    validate_config(config)
    set_random_seed(config.seed)
    _, grid_path, metadata_path = run_sdxl_seed_generation(
        config=config,
        file_prefix="q5_sdxl_ink",
        grid_file_name="q5_sdxl_ink_seed_comparison.png",
        metadata_file_name="q5_run_metadata.json",
        task_name="课程设计四 第五小问 SDXL Base 1.0 水墨风提示词多 seed 推理",
    )

    print("第五小问 SDXL 水墨风推理完成")
    print(f"对比图: {grid_path}")
    print(f"运行记录: {metadata_path}")


if __name__ == "__main__":
    main()
