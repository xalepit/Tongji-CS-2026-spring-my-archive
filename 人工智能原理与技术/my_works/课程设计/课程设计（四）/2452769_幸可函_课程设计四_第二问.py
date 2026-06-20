# -*- coding: utf-8 -*-
"""
课程设计四 第二小问：加载 jpg-csv 数据集并对 Stable Diffusion v1.5 进行 LoRA 微调。
"""

from __future__ import annotations

import csv
import os
import sys
from dataclasses import asdict, dataclass, field
from datetime import datetime
from pathlib import Path

import pandas as pd
from PIL import Image, ImageOps
from tqdm.auto import tqdm

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
    build_csv_dataloader,
    generate_lora_sample,
    load_csv_metadata,
    metadata_csv_is_valid,
    train_lora,
    validate_lora_training_config,
)


DEFAULT_DATASET_ROOT = r"D:\AI_Datasets\AmazarashiEndure_Chinese_Landscape_Painting"
DEFAULT_PREPARED_DIR = r"D:\AI_Datasets\AmazarashiEndure_Chinese_Landscape_Painting\prepared_jpg_csv"


@dataclass(frozen=True)
class TrainingConfig:
    """第二小问的路径、训练超参数和保存设置。"""

    model_id: str = MODEL_ID
    dataset_root: Path = field(
        default_factory=lambda: Path(
            os.environ.get("COURSE4_Q2_DATASET_ROOT", DEFAULT_DATASET_ROOT)
        )
    )
    prepared_dataset_dir: Path = field(
        default_factory=lambda: Path(
            os.environ.get("COURSE4_Q2_PREPARED_DIR", DEFAULT_PREPARED_DIR)
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
    jpeg_quality: int = 95
    rebuild_prepared_dataset: bool = False
    lora_weight_name: str = "q2_unet_lora.safetensors"
    validation_prompt: str = (
        "Chinese poem: 孤舟蓑笠翁，独钓寒江雪. "
        "Chinese ink wash landscape painting, xuan paper, black ink, "
        "misty mountains, snowy river, lone fisherman in a small boat, "
        "blank space, elegant brush strokes"
    )
    negative_prompt: str = (
        "low quality, blurry, photorealistic, colorful, cartoon, oil painting, "
        "modern city, car, crowd, text, watermark, signature, frame"
    )
    validation_width: int = 512
    validation_height: int = 512
    validation_steps: int = 30
    validation_guidance_scale: float = 7.5


def build_config() -> TrainingConfig:
    """构造第二小问运行配置。"""
    output_dir = make_output_dir(__file__, "question2")
    env_output_dir = os.environ.get("COURSE4_Q2_OUTPUT_DIR")
    if env_output_dir:
        output_dir = Path(env_output_dir)
    return TrainingConfig(output_dir=output_dir)


def validate_config(config: TrainingConfig) -> None:
    """检查路径、超参数和运行设备。"""
    if not config.dataset_root.is_dir():
        raise FileNotFoundError(f"数据集目录不存在: {config.dataset_root}")
    if not (config.dataset_root / "dataset").is_dir():
        raise FileNotFoundError(f"数据集原始文件目录不存在: {config.dataset_root / 'dataset'}")
    if config.resolution <= 0 or config.resolution % 8 != 0:
        raise ValueError("训练分辨率必须为正数且能被 8 整除。")
    if config.train_batch_size <= 0:
        raise ValueError("训练批量大小必须为正数。")
    if config.gradient_accumulation_steps <= 0:
        raise ValueError("梯度累积步数必须为正数。")
    if config.max_train_steps <= 0:
        raise ValueError("训练步数必须为正数。")
    if config.max_train_samples is not None and config.max_train_samples <= 0:
        raise ValueError("训练样本数量上限必须为正数。")
    validate_lora_training_config(config)


def read_caption(caption_path: Path) -> str:
    """读取并清理单张图片对应的文本描述。"""
    caption = caption_path.read_text(encoding="utf-8").strip()
    if not caption:
        raise ValueError(f"文本描述为空: {caption_path}")
    return " ".join(caption.split())


def make_jpg_name(image_path: Path, raw_root: Path) -> str:
    """根据原始相对路径生成稳定的 jpg 文件名。"""
    relative_path = image_path.relative_to(raw_root).with_suffix("")
    name_parts = [part for part in relative_path.parts if part]
    return "__".join(name_parts) + ".jpg"


def collect_raw_pairs(raw_root: Path) -> list[tuple[Path, Path]]:
    """收集原始 PNG 图片及同名 TXT 文本描述。"""
    image_paths = sorted(raw_root.rglob("*.png"))
    if not image_paths:
        raise FileNotFoundError(f"未找到 PNG 图像: {raw_root}")

    pairs: list[tuple[Path, Path]] = []
    missing_texts: list[Path] = []
    for image_path in image_paths:
        caption_path = image_path.with_suffix(".txt")
        if caption_path.is_file():
            pairs.append((image_path, caption_path))
        else:
            missing_texts.append(caption_path)

    if missing_texts:
        examples = ", ".join(str(path) for path in missing_texts[:5])
        raise FileNotFoundError(f"存在缺失文本描述的图片: {examples}")
    return pairs


def prepare_jpg_csv_dataset(config: TrainingConfig) -> Path:
    """将 PNG/TXT 原始数据整理为 jpg-csv 数据集。"""
    raw_root = config.dataset_root / "dataset"
    prepared_dir = config.prepared_dataset_dir
    image_dir = prepared_dir / "images"
    metadata_path = prepared_dir / "metadata.csv"

    if not config.rebuild_prepared_dataset and metadata_csv_is_valid(metadata_path, prepared_dir):
        return metadata_path

    pairs = collect_raw_pairs(raw_root)
    image_dir.mkdir(parents=True, exist_ok=True)
    records: list[dict[str, str]] = []

    for image_path, caption_path in tqdm(pairs, desc="整理 jpg-csv 数据集"):
        jpg_name = make_jpg_name(image_path, raw_root)
        jpg_path = image_dir / jpg_name
        if not jpg_path.is_file() or config.rebuild_prepared_dataset:
            with Image.open(image_path) as image:
                image = ImageOps.exif_transpose(image).convert("RGB")
                image.save(jpg_path, format="JPEG", quality=config.jpeg_quality, optimize=True)

        records.append(
            {
                "file_name": str(Path("images") / jpg_name).replace("\\", "/"),
                "text": read_caption(caption_path),
            }
        )

    prepared_dir.mkdir(parents=True, exist_ok=True)
    with metadata_path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.DictWriter(file, fieldnames=["file_name", "text"])
        writer.writeheader()
        writer.writerows(records)

    return metadata_path


def save_run_records(
    config: TrainingConfig,
    metadata_path: Path,
    metadata: pd.DataFrame,
    lora_path: Path,
    sample_path: Path,
    train_logs: list[dict[str, float]],
) -> tuple[Path, Path]:
    """保存训练日志和本次运行参数。"""
    config.output_dir.mkdir(parents=True, exist_ok=True)
    log_path = config.output_dir / "q2_train_log.csv"
    metadata_output_path = config.output_dir / "q2_run_metadata.json"

    pd.DataFrame(train_logs).to_csv(log_path, index=False, encoding="utf-8")

    config_dict = asdict(config)
    for key, value in config_dict.items():
        if isinstance(value, Path):
            config_dict[key] = str(value)

    run_metadata = {
        "task": "课程设计四 第二小问 Stable Diffusion v1.5 LoRA 微调",
        "time": datetime.now().isoformat(timespec="seconds"),
        "model_id": config.model_id,
        "dataset_root": str(config.dataset_root),
        "prepared_metadata": str(metadata_path),
        "prepared_sample_count": int(len(pd.read_csv(metadata_path))),
        "training_sample_count": int(len(metadata)),
        "lora_weight": str(lora_path),
        "sample_image": str(sample_path),
        "config": config_dict,
        "environment": get_runtime_environment(),
    }
    write_json(metadata_output_path, run_metadata)

    return log_path, metadata_output_path


def main() -> None:
    """执行数据整理、jpg-csv 加载、LoRA 微调和样例图生成。"""
    config = build_config()
    validate_config(config)
    set_random_seed(config.seed)
    config.output_dir.mkdir(parents=True, exist_ok=True)

    metadata_path = prepare_jpg_csv_dataset(config)
    metadata = load_csv_metadata(
        metadata_path=metadata_path,
        max_samples=config.max_train_samples,
        seed=config.seed,
    )
    lora_path, train_logs = train_lora(
        config=config,
        dataloader_builder=lambda tokenizer: build_csv_dataloader(
            metadata,
            metadata_path.parent,
            tokenizer,
            config,
        ),
        train_sample_count=len(metadata),
    )
    sample_path = generate_lora_sample(
        config=config,
        lora_path=lora_path,
        prompt=config.validation_prompt,
        negative_prompt=config.negative_prompt,
        image_path=config.output_dir / "samples" / "q2_lora_poem_ink_sample.png",
        width=config.validation_width,
        height=config.validation_height,
        num_inference_steps=config.validation_steps,
        guidance_scale=config.validation_guidance_scale,
    )
    log_path, metadata_output_path = save_run_records(
        config,
        metadata_path,
        metadata,
        lora_path,
        sample_path,
        train_logs,
    )

    print("第二小问 LoRA 微调完成")
    print(f"jpg-csv 数据集: {metadata_path}")
    print(f"LoRA 权重: {lora_path}")
    print(f"样例图片: {sample_path}")
    print(f"训练日志: {log_path}")
    print(f"运行记录: {metadata_output_path}")


if __name__ == "__main__":
    main()
