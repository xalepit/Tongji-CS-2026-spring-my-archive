# -*- coding: utf-8 -*-
"""
课程设计四 第六小问：使用 DreamBooth LoRA 对 SD1.5 进行低显存风格微调。
"""

from __future__ import annotations

import os
import sys
from dataclasses import asdict, dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any

import pandas as pd
import torch
from PIL import Image, ImageDraw

SCRIPT_DIR = Path(__file__).resolve().parent
if str(SCRIPT_DIR) not in sys.path:
    sys.path.insert(0, str(SCRIPT_DIR))

from utils.course4_common import (
    DEFAULT_SEED,
    MODEL_ID,
    get_runtime_environment,
    make_output_dir,
    set_random_seed,
    write_json,
)
from utils.course4_lora import (
    build_dreambooth_dataloader,
    generate_lora_sample,
    generate_sd15_sample,
    load_csv_metadata,
    train_lora,
    validate_lora_training_config,
)


DEFAULT_PREPARED_DIR = (
    r"D:\AI_Datasets\AmazarashiEndure_Chinese_Landscape_Painting\prepared_jpg_csv"
)


@dataclass(frozen=True)
class TrainingConfig:
    """第六小问的路径、DreamBooth LoRA 超参数和保存设置。"""

    model_id: str = MODEL_ID
    prepared_dataset_dir: Path = field(
        default_factory=lambda: Path(
            os.environ.get("COURSE4_Q6_PREPARED_DIR", DEFAULT_PREPARED_DIR)
        )
    )
    output_dir: Path = field(default_factory=Path)
    seed: int = DEFAULT_SEED
    resolution: int = 512
    train_batch_size: int = 1
    gradient_accumulation_steps: int = 4
    max_train_steps: int = 240
    max_train_samples: int = 24
    learning_rate: float = 1.0e-4
    adam_beta1: float = 0.9
    adam_beta2: float = 0.999
    adam_weight_decay: float = 1.0e-2
    adam_epsilon: float = 1.0e-8
    lora_rank: int = 8
    lora_alpha: int = 16
    lora_dropout: float = 0.05
    dataloader_num_workers: int = 0
    log_interval: int = 10
    lora_weight_name: str = "q6_dreambooth_unet_lora.safetensors"
    file_column: str = "file_name"
    text_column: str = "text"
    keyword_weights: tuple[tuple[str, int], ...] = (
        ("fishing", 6),
        ("fisherman", 6),
        ("boat", 5),
        ("river", 3),
        ("water", 2),
        ("misty", 1),
    )
    instance_prompt: str = (
        "qfhshuimo style, Chinese ink wash landscape painting, misty mountains, "
        "river, small boat, xuan paper, black ink, empty space"
    )
    validation_prompt: str = (
        "Poem: 孤舟蓑笠翁，独钓寒江雪. qfhshuimo style, "
        "Chinese ink wash painting, lone fisherman, bamboo hat, small boat, "
        "snowy river, misty mountains, black ink, empty space"
    )
    negative_prompt: str = (
        "low quality, blurry, photorealistic, colorful, cartoon, anime, "
        "oil painting, modern city, crowd, text, watermark"
    )
    validation_width: int = 512
    validation_height: int = 512
    validation_steps: int = 30
    validation_guidance_scale: float = 7.5
    dataset_name: str = "AmazarashiEndure/Chinese_Landscape_Painting"
    dataset_url: str = "https://huggingface.co/datasets/AmazarashiEndure/Chinese_Landscape_Painting"
    dataset_license: str = "cc-by-4.0"


def build_config() -> TrainingConfig:
    """构造第六小问运行配置。"""
    output_dir = make_output_dir(__file__, "question6")
    env_output_dir = os.environ.get("COURSE4_Q6_OUTPUT_DIR")
    if env_output_dir:
        output_dir = Path(env_output_dir)
    return TrainingConfig(output_dir=output_dir)


def validate_config(config: TrainingConfig) -> None:
    """检查路径、训练超参数和生成参数。"""
    metadata_path = config.prepared_dataset_dir / "metadata.csv"
    if not config.prepared_dataset_dir.is_dir():
        raise FileNotFoundError(f"数据集目录不存在: {config.prepared_dataset_dir}")
    if not metadata_path.is_file():
        raise FileNotFoundError(f"metadata.csv 不存在: {metadata_path}")
    if config.max_train_samples <= 0:
        raise ValueError("DreamBooth 训练样本数量必须为正数。")
    if config.validation_width % 8 != 0 or config.validation_height % 8 != 0:
        raise ValueError("生成图像宽高必须能被 8 整除。")
    if config.validation_steps <= 0 or config.validation_guidance_scale <= 0:
        raise ValueError("生成步数和 guidance scale 必须为正数。")
    validate_lora_training_config(config)


def score_metadata(metadata: pd.DataFrame, config: TrainingConfig) -> pd.Series:
    """根据目标意象关键词为样本打分。"""
    text = metadata[config.text_column].astype(str).str.lower()
    score = pd.Series(0, index=metadata.index, dtype="int64")
    for keyword, weight in config.keyword_weights:
        score += text.str.contains(keyword, regex=False).astype("int64") * weight
    return score


def select_dreambooth_metadata(config: TrainingConfig) -> tuple[pd.DataFrame, Path]:
    """选择少量适合 DreamBooth 风格绑定的水墨山水样本。"""
    metadata_path = config.prepared_dataset_dir / "metadata.csv"
    metadata = load_csv_metadata(
        metadata_path=metadata_path,
        max_samples=None,
        seed=config.seed,
        file_column=config.file_column,
        text_column=config.text_column,
    )
    metadata = metadata[
        metadata[config.file_column].map(
            lambda relative_path: (config.prepared_dataset_dir / str(relative_path)).is_file()
        )
    ].copy()
    if metadata.empty:
        raise ValueError("没有可用的 DreamBooth 训练样本。")

    metadata["_keyword_score"] = score_metadata(metadata, config)
    candidates = metadata[metadata["_keyword_score"] > 0].copy()
    if len(candidates) < config.max_train_samples:
        candidates = metadata.copy()

    selected = candidates.sort_values(
        by=["_keyword_score", config.file_column],
        ascending=[False, True],
        kind="mergesort",
    ).head(config.max_train_samples)
    selected = selected[[config.file_column, config.text_column, "_keyword_score"]]
    selected_metadata_path = config.output_dir / "q6_selected_dreambooth_metadata.csv"
    config.output_dir.mkdir(parents=True, exist_ok=True)
    selected.to_csv(selected_metadata_path, index=False, encoding="utf-8")
    return selected[[config.file_column, config.text_column]], selected_metadata_path


def create_before_after_grid(before_path: Path, after_path: Path, output_path: Path) -> Path:
    """拼接微调前后生成效果。"""
    output_path.parent.mkdir(parents=True, exist_ok=True)
    label_height = 34
    padding = 16

    with Image.open(before_path) as before_image, Image.open(after_path) as after_image:
        before_image = before_image.convert("RGB")
        after_image = after_image.convert("RGB")
        width, height = before_image.size
        if after_image.size != before_image.size:
            after_image = after_image.resize(before_image.size, Image.Resampling.LANCZOS)

        grid = Image.new(
            "RGB",
            (width * 2 + padding * 3, height + label_height + padding * 2),
            color=(245, 245, 245),
        )
        draw = ImageDraw.Draw(grid)
        before_left = padding
        after_left = width + padding * 2
        top = padding + label_height

        draw.text((before_left, padding + 8), "before SD1.5", fill=(20, 20, 20))
        draw.text((after_left, padding + 8), "after DreamBooth LoRA", fill=(20, 20, 20))
        grid.paste(before_image, (before_left, top))
        grid.paste(after_image, (after_left, top))

    grid.save(output_path)
    return output_path


def get_peak_cuda_memory_mb() -> float | None:
    """读取 CUDA 峰值显存占用。"""
    if not torch.cuda.is_available():
        return None
    return round(torch.cuda.max_memory_allocated() / 1024 / 1024, 2)


def save_run_records(
    config: TrainingConfig,
    selected_metadata_path: Path,
    selected_metadata: pd.DataFrame,
    lora_path: Path,
    before_path: Path,
    after_path: Path,
    comparison_path: Path,
    train_logs: list[dict[str, float]],
    peak_training_memory_mb: float | None,
) -> tuple[Path, Path]:
    """保存训练日志、运行参数和输出路径。"""
    config.output_dir.mkdir(parents=True, exist_ok=True)
    log_path = config.output_dir / "q6_train_log.csv"
    metadata_output_path = config.output_dir / "q6_run_metadata.json"

    pd.DataFrame(train_logs).to_csv(log_path, index=False, encoding="utf-8")

    config_dict = asdict(config)
    for key, value in config_dict.items():
        if isinstance(value, Path):
            config_dict[key] = str(value)

    run_metadata: dict[str, Any] = {
        "task": "课程设计四 第六小问 Stable Diffusion v1.5 DreamBooth LoRA 微调",
        "time": datetime.now().isoformat(timespec="seconds"),
        "model_id": config.model_id,
        "dataset_name": config.dataset_name,
        "dataset_url": config.dataset_url,
        "dataset_license": config.dataset_license,
        "prepared_dataset_dir": str(config.prepared_dataset_dir),
        "selected_metadata": str(selected_metadata_path),
        "training_sample_count": int(len(selected_metadata)),
        "instance_prompt": config.instance_prompt,
        "validation_prompt": config.validation_prompt,
        "lora_weight": str(lora_path),
        "lora_weight_size_bytes": int(lora_path.stat().st_size),
        "before_image": str(before_path),
        "after_image": str(after_path),
        "comparison_image": str(comparison_path),
        "peak_training_memory_mb": peak_training_memory_mb,
        "train_text_encoder": False,
        "config": config_dict,
        "environment": get_runtime_environment(),
    }
    write_json(metadata_output_path, run_metadata)
    return log_path, metadata_output_path


def main() -> None:
    """执行 DreamBooth LoRA 微调并生成微调前后对比图。"""
    config = build_config()
    validate_config(config)
    set_random_seed(config.seed)
    config.output_dir.mkdir(parents=True, exist_ok=True)

    selected_metadata, selected_metadata_path = select_dreambooth_metadata(config)
    before_path = generate_sd15_sample(
        config=config,
        prompt=config.validation_prompt,
        negative_prompt=config.negative_prompt,
        image_path=config.output_dir / "samples" / "q6_before_dreambooth.png",
        width=config.validation_width,
        height=config.validation_height,
        num_inference_steps=config.validation_steps,
        guidance_scale=config.validation_guidance_scale,
    )

    if torch.cuda.is_available():
        torch.cuda.reset_peak_memory_stats()

    lora_path, train_logs = train_lora(
        config=config,
        dataloader_builder=lambda tokenizer: build_dreambooth_dataloader(
            metadata=selected_metadata,
            dataset_dir=config.prepared_dataset_dir,
            tokenizer=tokenizer,
            config=config,
            instance_prompt=config.instance_prompt,
        ),
        train_sample_count=len(selected_metadata),
    )
    peak_training_memory_mb = get_peak_cuda_memory_mb()

    after_path = generate_lora_sample(
        config=config,
        lora_path=lora_path,
        prompt=config.validation_prompt,
        negative_prompt=config.negative_prompt,
        image_path=config.output_dir / "samples" / "q6_after_dreambooth_lora.png",
        width=config.validation_width,
        height=config.validation_height,
        num_inference_steps=config.validation_steps,
        guidance_scale=config.validation_guidance_scale,
    )
    comparison_path = create_before_after_grid(
        before_path=before_path,
        after_path=after_path,
        output_path=config.output_dir / "samples" / "q6_before_after_comparison.png",
    )
    log_path, metadata_output_path = save_run_records(
        config=config,
        selected_metadata_path=selected_metadata_path,
        selected_metadata=selected_metadata,
        lora_path=lora_path,
        before_path=before_path,
        after_path=after_path,
        comparison_path=comparison_path,
        train_logs=train_logs,
        peak_training_memory_mb=peak_training_memory_mb,
    )

    print("第六小问 DreamBooth LoRA 微调完成")
    print(f"训练样本记录: {selected_metadata_path}")
    print(f"LoRA 权重: {lora_path}")
    print(f"微调前图片: {before_path}")
    print(f"微调后图片: {after_path}")
    print(f"对比图: {comparison_path}")
    print(f"训练日志: {log_path}")
    print(f"运行记录: {metadata_output_path}")


if __name__ == "__main__":
    main()
