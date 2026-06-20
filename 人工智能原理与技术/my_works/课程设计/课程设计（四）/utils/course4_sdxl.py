# -*- coding: utf-8 -*-
"""
课程设计四 SDXL 推理公共函数。
"""

from __future__ import annotations

from dataclasses import asdict
from datetime import datetime
from pathlib import Path
from typing import Any, Protocol

import torch
from diffusers import DPMSolverMultistepScheduler, StableDiffusionXLPipeline
from PIL import Image, ImageDraw

from utils.course4_common import get_runtime_environment, write_json


SDXL_MODEL_ID = "stabilityai/stable-diffusion-xl-base-1.0"
DEFAULT_SDXL_MODEL_PATH = r"D:\AI_Models\stabilityai_stable-diffusion-xl-base-1.0"


class SdxlGenerationConfig(Protocol):
    """SDXL 多 seed 推理需要读取的配置字段。"""

    model_id: str
    output_dir: Path
    seeds: tuple[int, ...]
    width: int
    height: int
    num_inference_steps: int
    guidance_scale: float
    local_files_only: bool
    enable_cpu_offload: bool
    grid_columns: int
    grid_thumbnail_width: int
    prompt: str
    negative_prompt: str


def validate_sdxl_generation_config(config: SdxlGenerationConfig) -> None:
    """检查 SDXL 推理参数和 CUDA 运行条件。"""
    if not torch.cuda.is_available():
        raise RuntimeError("当前未检测到 CUDA，SDXL 推理需要 CUDA 环境。")
    model_path = Path(config.model_id)
    if config.local_files_only and model_path.drive and not model_path.is_dir():
        raise FileNotFoundError(f"本地 SDXL 模型目录不存在: {model_path}")
    if not config.seeds:
        raise ValueError("seed 列表不能为空。")
    if config.width <= 0 or config.height <= 0:
        raise ValueError("生成图像尺寸必须为正数。")
    if config.width % 8 != 0 or config.height % 8 != 0:
        raise ValueError("生成图像宽高必须能被 8 整除。")
    if config.num_inference_steps <= 0:
        raise ValueError("推理步数必须为正数。")
    if config.guidance_scale <= 0:
        raise ValueError("guidance scale 必须为正数。")
    if config.grid_columns <= 0 or config.grid_thumbnail_width <= 0:
        raise ValueError("拼图列数和缩略图宽度必须为正数。")


def load_sdxl_pipeline(config: SdxlGenerationConfig) -> StableDiffusionXLPipeline:
    """加载 SDXL Base 1.0 推理管线。"""
    pipe = StableDiffusionXLPipeline.from_pretrained(
        config.model_id,
        torch_dtype=torch.float16,
        variant="fp16",
        use_safetensors=True,
        local_files_only=config.local_files_only,
    )
    pipe.scheduler = DPMSolverMultistepScheduler.from_config(
        pipe.scheduler.config,
        use_karras_sigmas=True,
        algorithm_type="sde-dpmsolver++",
    )
    pipe.vae.enable_slicing()

    if config.enable_cpu_offload:
        try:
            pipe.enable_model_cpu_offload()
            return pipe
        except (ImportError, RuntimeError, ValueError):
            pass

    pipe = pipe.to("cuda")
    return pipe


def generate_single_image(
    pipe: StableDiffusionXLPipeline,
    config: SdxlGenerationConfig,
    seed: int,
    image_path: Path,
) -> Path:
    """按指定 seed 生成并保存单张候选图。"""
    image_path.parent.mkdir(parents=True, exist_ok=True)
    generator = torch.Generator(device="cuda").manual_seed(seed)
    with torch.inference_mode():
        image = pipe(
            prompt=config.prompt,
            negative_prompt=config.negative_prompt,
            width=config.width,
            height=config.height,
            num_inference_steps=config.num_inference_steps,
            guidance_scale=config.guidance_scale,
            generator=generator,
        ).images[0]
    image.save(image_path)
    return image_path


def generate_candidate_images(
    pipe: StableDiffusionXLPipeline,
    config: SdxlGenerationConfig,
    file_prefix: str,
) -> list[dict[str, Any]]:
    """生成多 seed 候选图并记录参数。"""
    sample_dir = config.output_dir / "samples"
    records: list[dict[str, Any]] = []
    for index, seed in enumerate(config.seeds, start=1):
        image_path = sample_dir / f"{file_prefix}_seed_{seed}.png"
        generate_single_image(pipe, config, seed, image_path)
        records.append(
            {
                "index": index,
                "seed": seed,
                "image_path": str(image_path),
                "width": config.width,
                "height": config.height,
                "num_inference_steps": config.num_inference_steps,
                "guidance_scale": config.guidance_scale,
            }
        )
        print(f"生成完成: seed={seed}, image={image_path}")
    return records


def create_seed_grid(
    records: list[dict[str, Any]],
    config: SdxlGenerationConfig,
    grid_path: Path,
) -> Path:
    """将多 seed 候选图拼接为对比图。"""
    if not records:
        raise ValueError("候选图记录不能为空。")

    grid_path.parent.mkdir(parents=True, exist_ok=True)
    columns = min(config.grid_columns, len(records))
    rows = (len(records) + columns - 1) // columns
    thumbnail_width = config.grid_thumbnail_width
    thumbnail_height = round(thumbnail_width * config.height / config.width)
    label_height = 34
    padding = 16
    cell_width = thumbnail_width + padding * 2
    cell_height = thumbnail_height + label_height + padding * 2

    grid = Image.new(
        "RGB",
        (columns * cell_width, rows * cell_height),
        color=(245, 245, 245),
    )
    draw = ImageDraw.Draw(grid)

    for index, record in enumerate(records):
        row = index // columns
        column = index % columns
        left = column * cell_width + padding
        top = row * cell_height + padding
        with Image.open(record["image_path"]) as image:
            image = image.convert("RGB")
            image = image.resize((thumbnail_width, thumbnail_height), Image.Resampling.LANCZOS)
            grid.paste(image, (left, top + label_height))
        label = (
            f"seed={record['seed']}  steps={record['num_inference_steps']}  "
            f"guidance={record['guidance_scale']}"
        )
        draw.text((left, top + 8), label, fill=(20, 20, 20))

    grid.save(grid_path)
    return grid_path


def save_sdxl_run_records(
    config: SdxlGenerationConfig,
    candidate_records: list[dict[str, Any]],
    grid_path: Path,
    metadata_path: Path,
    task_name: str,
) -> Path:
    """保存 SDXL 运行参数和输出路径。"""
    config_dict = asdict(config)
    for key, value in config_dict.items():
        if isinstance(value, Path):
            config_dict[key] = str(value)

    metadata = {
        "task": task_name,
        "time": datetime.now().isoformat(timespec="seconds"),
        "base_model_id": SDXL_MODEL_ID,
        "model_path": config.model_id,
        "prompt": config.prompt,
        "negative_prompt": config.negative_prompt,
        "candidate_images": candidate_records,
        "comparison_grid": str(grid_path),
        "config": config_dict,
        "environment": get_runtime_environment(),
    }
    write_json(metadata_path, metadata)
    return metadata_path


def run_sdxl_seed_generation(
    config: SdxlGenerationConfig,
    file_prefix: str,
    grid_file_name: str,
    metadata_file_name: str,
    task_name: str,
) -> tuple[list[dict[str, Any]], Path, Path]:
    """执行 SDXL 多 seed 推理并生成对比图和运行记录。"""
    validate_sdxl_generation_config(config)
    config.output_dir.mkdir(parents=True, exist_ok=True)

    pipe = load_sdxl_pipeline(config)
    pipe.set_progress_bar_config(disable=False)
    candidate_records = generate_candidate_images(pipe, config, file_prefix)
    grid_path = create_seed_grid(
        candidate_records,
        config,
        config.output_dir / grid_file_name,
    )
    metadata_path = save_sdxl_run_records(
        config,
        candidate_records,
        grid_path,
        config.output_dir / metadata_file_name,
        task_name,
    )

    del pipe
    torch.cuda.empty_cache()
    return candidate_records, grid_path, metadata_path
