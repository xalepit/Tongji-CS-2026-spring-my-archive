# -*- coding: utf-8 -*-
"""
课程设计四 第三小问：加载写实风格 parquet 数据集并进行 LoRA 微调。
"""

from __future__ import annotations

import os
import sys
from dataclasses import asdict, dataclass, field
from datetime import datetime
from pathlib import Path

import pandas as pd
from datasets import Dataset as HFDataset
from datasets import Image as HFImage
from datasets import load_dataset

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
    ParquetImageTextDataset,
    build_dataloader,
    caption_has_text,
    generate_lora_sample,
    parquet_image_is_readable,
    train_lora,
    validate_lora_training_config,
)


DEFAULT_DATASET_ROOT = r"D:\AI_Datasets\KoalaAI_StockImages_CC0"


@dataclass(frozen=True)
class TrainingConfig:
    """第三小问的路径、训练超参数和保存设置。"""

    model_id: str = MODEL_ID
    dataset_root: Path = field(
        default_factory=lambda: Path(
            os.environ.get("COURSE4_Q3_DATASET_ROOT", DEFAULT_DATASET_ROOT)
        )
    )
    output_dir: Path = field(default_factory=Path)
    seed: int = DEFAULT_SEED
    resolution: int = 512
    train_batch_size: int = 1
    gradient_accumulation_steps: int = 4
    max_train_steps: int = 200
    max_train_samples: int | None = 512
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
    image_column: str = "image"
    text_column: str = "tags"
    text_prefix: str = "realistic photo, stock photography"
    lora_weight_name: str = "q3_unet_lora.safetensors"
    validation_prompt: str = (
        "Chinese poem: 孤舟蓑笠翁，独钓寒江雪. "
        "realistic photo, cinematic winter landscape, snowy river, "
        "lone fisherman in a small boat, misty mountains, natural light, "
        "high detail, documentary photography"
    )
    negative_prompt: str = (
        "low quality, blurry, cartoon, anime, illustration, oil painting, "
        "ink wash painting, watercolor, text, watermark, signature, frame"
    )
    validation_width: int = 512
    validation_height: int = 512
    validation_steps: int = 30
    validation_guidance_scale: float = 7.5


def build_config() -> TrainingConfig:
    """构造第三小问运行配置。"""
    output_dir = make_output_dir(__file__, "question3")
    env_output_dir = os.environ.get("COURSE4_Q3_OUTPUT_DIR")
    if env_output_dir:
        output_dir = Path(env_output_dir)
    return TrainingConfig(output_dir=output_dir)


def get_parquet_files(dataset_root: Path) -> list[str]:
    """获取本地 parquet 数据文件。"""
    parquet_files = sorted((dataset_root / "data").glob("*.parquet"))
    if not parquet_files:
        raise FileNotFoundError(f"未找到 parquet 数据文件: {dataset_root / 'data'}")
    return [str(path) for path in parquet_files]


def validate_config(config: TrainingConfig) -> None:
    """检查路径、字段和训练超参数。"""
    if not config.dataset_root.is_dir():
        raise FileNotFoundError(f"数据集目录不存在: {config.dataset_root}")
    get_parquet_files(config.dataset_root)
    if config.max_train_samples is not None and config.max_train_samples <= 0:
        raise ValueError("训练样本数量上限必须为正数。")
    validate_lora_training_config(config)


def sample_is_valid(row: dict[str, object], config: TrainingConfig) -> bool:
    """检查 parquet 样本是否具备可训练的图像和文本。"""
    return caption_has_text(row[config.text_column]) and parquet_image_is_readable(
        row[config.image_column]
    )


def load_realistic_dataset(config: TrainingConfig) -> HFDataset:
    """加载写实风格 parquet 图文数据集。"""
    data_files = get_parquet_files(config.dataset_root)
    dataset = load_dataset("parquet", data_files=data_files, split="train")
    required_columns = {config.image_column, config.text_column}
    if not required_columns.issubset(dataset.column_names):
        raise ValueError("parquet 数据集必须包含图像字段和文本字段。")

    dataset = dataset.cast_column(config.image_column, HFImage(decode=False))
    dataset = dataset.filter(
        lambda row: sample_is_valid(row, config),
        desc="过滤无效训练样本",
    )
    if len(dataset) == 0:
        raise ValueError("parquet 数据集中没有可用训练样本。")

    if config.max_train_samples is not None and len(dataset) > config.max_train_samples:
        dataset = dataset.shuffle(seed=config.seed).select(range(config.max_train_samples))
    return dataset


def save_run_records(
    config: TrainingConfig,
    dataset: HFDataset,
    lora_path: Path,
    sample_path: Path,
    train_logs: list[dict[str, float]],
) -> tuple[Path, Path]:
    """保存训练日志和本次运行参数。"""
    config.output_dir.mkdir(parents=True, exist_ok=True)
    log_path = config.output_dir / "q3_train_log.csv"
    metadata_output_path = config.output_dir / "q3_run_metadata.json"

    pd.DataFrame(train_logs).to_csv(log_path, index=False, encoding="utf-8")

    config_dict = asdict(config)
    for key, value in config_dict.items():
        if isinstance(value, Path):
            config_dict[key] = str(value)

    run_metadata = {
        "task": "课程设计四 第三小问 Stable Diffusion v1.5 写实风格 LoRA 微调",
        "time": datetime.now().isoformat(timespec="seconds"),
        "model_id": config.model_id,
        "dataset_root": str(config.dataset_root),
        "dataset_name": "KoalaAI/StockImages-CC0",
        "dataset_format": "parquet",
        "image_column": config.image_column,
        "text_column": config.text_column,
        "training_sample_count": int(len(dataset)),
        "lora_weight": str(lora_path),
        "sample_image": str(sample_path),
        "config": config_dict,
        "environment": get_runtime_environment(),
    }
    write_json(metadata_output_path, run_metadata)

    return log_path, metadata_output_path


def main() -> None:
    """执行 parquet 加载、LoRA 微调和写实风格样例图生成。"""
    config = build_config()
    validate_config(config)
    set_random_seed(config.seed)
    config.output_dir.mkdir(parents=True, exist_ok=True)

    dataset = load_realistic_dataset(config)
    lora_path, train_logs = train_lora(
        config=config,
        dataloader_builder=lambda tokenizer: build_dataloader(
            ParquetImageTextDataset(
                dataset=dataset,
                tokenizer=tokenizer,
                resolution=config.resolution,
                image_column=config.image_column,
                text_column=config.text_column,
                text_prefix=config.text_prefix,
            ),
            config,
        ),
        train_sample_count=len(dataset),
    )
    sample_path = generate_lora_sample(
        config=config,
        lora_path=lora_path,
        prompt=config.validation_prompt,
        negative_prompt=config.negative_prompt,
        image_path=config.output_dir / "samples" / "q3_lora_poem_realistic_sample.png",
        width=config.validation_width,
        height=config.validation_height,
        num_inference_steps=config.validation_steps,
        guidance_scale=config.validation_guidance_scale,
    )
    log_path, metadata_output_path = save_run_records(
        config,
        dataset,
        lora_path,
        sample_path,
        train_logs,
    )

    print("第三小问 LoRA 微调完成")
    print(f"parquet 数据集: {config.dataset_root}")
    print(f"LoRA 权重: {lora_path}")
    print(f"样例图片: {sample_path}")
    print(f"训练日志: {log_path}")
    print(f"运行记录: {metadata_output_path}")


if __name__ == "__main__":
    main()
