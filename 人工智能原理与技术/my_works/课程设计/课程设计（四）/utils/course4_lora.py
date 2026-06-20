# -*- coding: utf-8 -*-
"""
课程设计四 LoRA 微调公共函数。
"""

from __future__ import annotations

import io
from collections.abc import Mapping
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from typing import Any, Callable, Protocol

import pandas as pd
import torch
import torch.nn.functional as F
from diffusers import (
    AutoencoderKL,
    DDPMScheduler,
    DPMSolverMultistepScheduler,
    StableDiffusionPipeline,
    UNet2DConditionModel,
)
from diffusers.utils import convert_state_dict_to_diffusers
from diffusers.utils import logging as diffusers_logging
from peft import LoraConfig, get_peft_model_state_dict
from PIL import Image, ImageOps
from torch.utils.data import DataLoader, Dataset
from torchvision import transforms
from torchvision.transforms import InterpolationMode
from tqdm.auto import tqdm
from transformers import CLIPTextModel, CLIPTokenizer

from utils.course4_common import count_parameters


class LoraTrainingConfig(Protocol):
    """LoRA 微调流程需要读取的配置字段。"""

    model_id: str
    output_dir: Path
    seed: int
    resolution: int
    train_batch_size: int
    gradient_accumulation_steps: int
    max_train_steps: int
    learning_rate: float
    adam_beta1: float
    adam_beta2: float
    adam_weight_decay: float
    adam_epsilon: float
    lora_rank: int
    lora_alpha: int
    lora_dropout: float
    dataloader_num_workers: int
    log_interval: int
    lora_weight_name: str


class CsvImageTextDataset(Dataset[dict[str, torch.Tensor]]):
    """从 metadata.csv 加载图像路径和文本描述。"""

    def __init__(
        self,
        metadata: pd.DataFrame,
        dataset_dir: Path,
        tokenizer: CLIPTokenizer,
        resolution: int,
        file_column: str = "file_name",
        text_column: str = "text",
    ) -> None:
        self.metadata = metadata.reset_index(drop=True)
        self.dataset_dir = dataset_dir
        self.tokenizer = tokenizer
        self.file_column = file_column
        self.text_column = text_column
        self.image_transform = build_image_transform(resolution)

    def __len__(self) -> int:
        return len(self.metadata)

    def __getitem__(self, index: int) -> dict[str, torch.Tensor]:
        row = self.metadata.iloc[index]
        image_path = self.dataset_dir / str(row[self.file_column])
        caption = str(row[self.text_column])

        if not image_path.is_file():
            raise FileNotFoundError(f"图像文件不存在: {image_path}")

        pixel_values = load_transformed_image(image_path, self.image_transform)
        input_ids = tokenize_caption(self.tokenizer, caption)
        return {
            "pixel_values": pixel_values,
            "input_ids": input_ids,
        }


class ParquetImageTextDataset(Dataset[dict[str, torch.Tensor]]):
    """从 parquet 数据集对象加载图像和文本描述。"""

    def __init__(
        self,
        dataset: Any,
        tokenizer: CLIPTokenizer,
        resolution: int,
        image_column: str,
        text_column: str,
        text_prefix: str = "",
    ) -> None:
        self.dataset = dataset
        self.tokenizer = tokenizer
        self.image_column = image_column
        self.text_column = text_column
        self.text_prefix = text_prefix.strip()
        self.image_transform = build_image_transform(resolution)

    def __len__(self) -> int:
        return len(self.dataset)

    def __getitem__(self, index: int) -> dict[str, torch.Tensor]:
        row = self.dataset[index]
        image = load_parquet_image(row[self.image_column])
        caption = normalize_caption(row[self.text_column], self.text_prefix)

        pixel_values = self.image_transform(image)
        input_ids = tokenize_caption(self.tokenizer, caption)
        return {
            "pixel_values": pixel_values,
            "input_ids": input_ids,
        }


class DreamBoothImageDataset(Dataset[dict[str, torch.Tensor]]):
    """使用固定实例提示词加载 DreamBooth 图像样本。"""

    def __init__(
        self,
        metadata: pd.DataFrame,
        dataset_dir: Path,
        tokenizer: CLIPTokenizer,
        resolution: int,
        instance_prompt: str,
        file_column: str = "file_name",
    ) -> None:
        self.metadata = metadata.reset_index(drop=True)
        self.dataset_dir = dataset_dir
        self.tokenizer = tokenizer
        self.instance_prompt = instance_prompt
        self.file_column = file_column
        self.image_transform = build_image_transform(resolution)

    def __len__(self) -> int:
        return len(self.metadata)

    def __getitem__(self, index: int) -> dict[str, torch.Tensor]:
        row = self.metadata.iloc[index]
        image_path = self.dataset_dir / str(row[self.file_column])
        if not image_path.is_file():
            raise FileNotFoundError(f"图像文件不存在: {image_path}")

        pixel_values = load_transformed_image(image_path, self.image_transform)
        input_ids = tokenize_caption(self.tokenizer, self.instance_prompt)
        return {
            "pixel_values": pixel_values,
            "input_ids": input_ids,
        }


def build_image_transform(resolution: int) -> transforms.Compose:
    """构造 Stable Diffusion 训练图像预处理流程。"""
    return transforms.Compose(
        [
            transforms.Resize(
                resolution,
                interpolation=InterpolationMode.BICUBIC,
                antialias=True,
            ),
            transforms.CenterCrop(resolution),
            transforms.ToTensor(),
            transforms.Normalize([0.5, 0.5, 0.5], [0.5, 0.5, 0.5]),
        ]
    )


def load_transformed_image(image_path: Path, image_transform: transforms.Compose) -> torch.Tensor:
    """读取图像并应用训练预处理。"""
    with Image.open(image_path) as image:
        image = ImageOps.exif_transpose(image).convert("RGB")
        return image_transform(image)


def tokenize_caption(tokenizer: CLIPTokenizer, caption: str) -> torch.Tensor:
    """将文本描述转换为 CLIP token。"""
    tokenized = tokenizer(
        caption,
        max_length=tokenizer.model_max_length,
        padding="max_length",
        truncation=True,
        return_tensors="pt",
    )
    return tokenized.input_ids[0]


def clean_caption_text(caption: Any) -> str:
    """清理文本描述中的空白和成对引号。"""
    if caption is None:
        return ""
    caption_text = " ".join(str(caption).strip().split())
    if (
        len(caption_text) >= 2
        and caption_text[0] == caption_text[-1]
        and caption_text[0] in {"'", '"'}
    ):
        caption_text = caption_text[1:-1].strip()
    return caption_text


def caption_has_text(caption: Any) -> bool:
    """判断文本描述是否包含有效字符。"""
    return bool(clean_caption_text(caption))


def normalize_caption(caption: Any, prefix: str = "") -> str:
    """清理文本描述并按需添加风格前缀。"""
    caption = clean_caption_text(caption)
    if not caption:
        raise ValueError("文本描述为空。")
    if prefix:
        return f"{prefix}, {caption}"
    return caption


def load_parquet_image(image_value: Any) -> Image.Image:
    """从 parquet 图像字段读取 RGB 图像。"""
    if isinstance(image_value, Image.Image):
        return ImageOps.exif_transpose(image_value).convert("RGB")

    if isinstance(image_value, Mapping):
        image_bytes = image_value.get("bytes")
        if image_bytes:
            with Image.open(io.BytesIO(image_bytes)) as image:
                return ImageOps.exif_transpose(image).convert("RGB")

        image_path = image_value.get("path")
        if image_path:
            with Image.open(image_path) as image:
                return ImageOps.exif_transpose(image).convert("RGB")

    raise TypeError("parquet 图像字段无法读取为 PIL Image。")


def parquet_image_is_readable(image_value: Any) -> bool:
    """检查 parquet 图像字段是否可以被 PIL 正常解码。"""
    try:
        image = load_parquet_image(image_value)
        image.close()
    except (OSError, TypeError, ValueError):
        return False
    return True


def validate_lora_training_config(config: LoraTrainingConfig) -> None:
    """检查通用 LoRA 训练超参数和运行设备。"""
    if config.resolution <= 0 or config.resolution % 8 != 0:
        raise ValueError("训练分辨率必须为正数且能被 8 整除。")
    if config.train_batch_size <= 0:
        raise ValueError("训练批量大小必须为正数。")
    if config.gradient_accumulation_steps <= 0:
        raise ValueError("梯度累积步数必须为正数。")
    if config.max_train_steps <= 0:
        raise ValueError("训练步数必须为正数。")
    if config.lora_rank <= 0 or config.lora_alpha <= 0:
        raise ValueError("LoRA 秩和缩放系数必须为正数。")
    if not torch.cuda.is_available():
        raise RuntimeError("当前未检测到 CUDA，微调过程需要 CUDA 环境。")


def load_csv_metadata(
    metadata_path: Path,
    max_samples: int | None,
    seed: int,
    file_column: str = "file_name",
    text_column: str = "text",
) -> pd.DataFrame:
    """读取图文 metadata.csv，并按配置截取训练样本。"""
    metadata = pd.read_csv(metadata_path)
    required_columns = {file_column, text_column}
    if not required_columns.issubset(metadata.columns):
        raise ValueError("metadata.csv 必须包含图像路径列和文本描述列。")

    metadata = metadata[[file_column, text_column]].dropna()
    metadata[file_column] = metadata[file_column].astype(str).str.strip()
    metadata[text_column] = metadata[text_column].astype(str).str.strip()
    metadata = metadata[(metadata[file_column] != "") & (metadata[text_column] != "")]

    if metadata.empty:
        raise ValueError("metadata.csv 中没有可用训练样本。")

    if max_samples is not None and len(metadata) > max_samples:
        if max_samples <= 0:
            raise ValueError("训练样本数量上限必须为正数。")
        metadata = metadata.sample(n=max_samples, random_state=seed)
        metadata = metadata.sort_index()

    return metadata.reset_index(drop=True)


def metadata_csv_is_valid(metadata_path: Path, dataset_dir: Path) -> bool:
    """检查已有 jpg-csv 数据集是否可直接复用。"""
    if not metadata_path.is_file():
        return False
    try:
        metadata = pd.read_csv(metadata_path)
    except (OSError, pd.errors.ParserError, UnicodeDecodeError):
        return False

    if not {"file_name", "text"}.issubset(metadata.columns):
        return False
    if metadata.empty:
        return False

    for relative_name in metadata["file_name"].astype(str):
        if not (dataset_dir / relative_name).is_file():
            return False
    return True


def collate_image_text_batch(batch: list[dict[str, torch.Tensor]]) -> dict[str, torch.Tensor]:
    """合并一个批次内的图像张量和文本 token。"""
    pixel_values = torch.stack([item["pixel_values"] for item in batch])
    input_ids = torch.stack([item["input_ids"] for item in batch])
    return {
        "pixel_values": pixel_values.contiguous(),
        "input_ids": input_ids.contiguous(),
    }


def build_csv_dataloader(
    metadata: pd.DataFrame,
    dataset_dir: Path,
    tokenizer: CLIPTokenizer,
    config: LoraTrainingConfig,
) -> DataLoader[dict[str, torch.Tensor]]:
    """根据 jpg-csv 元数据构造训练 DataLoader。"""
    dataset = CsvImageTextDataset(
        metadata=metadata,
        dataset_dir=dataset_dir,
        tokenizer=tokenizer,
        resolution=config.resolution,
    )
    return build_dataloader(dataset, config)


def build_dreambooth_dataloader(
    metadata: pd.DataFrame,
    dataset_dir: Path,
    tokenizer: CLIPTokenizer,
    config: LoraTrainingConfig,
    instance_prompt: str,
) -> DataLoader[dict[str, torch.Tensor]]:
    """根据固定实例提示词构造 DreamBooth 训练 DataLoader。"""
    dataset = DreamBoothImageDataset(
        metadata=metadata,
        dataset_dir=dataset_dir,
        tokenizer=tokenizer,
        resolution=config.resolution,
        instance_prompt=instance_prompt,
    )
    return build_dataloader(dataset, config)


def build_dataloader(
    dataset: Dataset[dict[str, torch.Tensor]],
    config: LoraTrainingConfig,
) -> DataLoader[dict[str, torch.Tensor]]:
    """构造可复现的图文训练 DataLoader。"""
    generator = torch.Generator().manual_seed(config.seed)
    return DataLoader(
        dataset,
        batch_size=config.train_batch_size,
        shuffle=True,
        num_workers=config.dataloader_num_workers,
        collate_fn=collate_image_text_batch,
        generator=generator,
        drop_last=False,
    )


def cast_trainable_parameters(model: torch.nn.Module, dtype: torch.dtype) -> None:
    """将可训练参数保持为 float32，减少半精度训练中的数值误差。"""
    for parameter in model.parameters():
        if parameter.requires_grad:
            parameter.data = parameter.data.to(dtype=dtype)


def load_training_modules(
    config: LoraTrainingConfig,
    device: torch.device,
    weight_dtype: torch.dtype,
) -> tuple[DDPMScheduler, CLIPTokenizer, CLIPTextModel, AutoencoderKL, UNet2DConditionModel]:
    """加载 SD1.5 训练所需的调度器、分词器、文本编码器、VAE 和 U-Net。"""
    load_kwargs: dict[str, Any] = {
        "local_files_only": True,
    }
    fp16_kwargs: dict[str, Any] = {
        "torch_dtype": weight_dtype,
        "variant": "fp16",
        "local_files_only": True,
    }

    noise_scheduler = DDPMScheduler.from_pretrained(
        config.model_id,
        subfolder="scheduler",
        **load_kwargs,
    )
    tokenizer = CLIPTokenizer.from_pretrained(
        config.model_id,
        subfolder="tokenizer",
        **load_kwargs,
    )
    text_encoder = CLIPTextModel.from_pretrained(
        config.model_id,
        subfolder="text_encoder",
        **fp16_kwargs,
    )
    vae = AutoencoderKL.from_pretrained(
        config.model_id,
        subfolder="vae",
        **fp16_kwargs,
    )
    unet = UNet2DConditionModel.from_pretrained(
        config.model_id,
        subfolder="unet",
        **fp16_kwargs,
    )

    text_encoder.requires_grad_(False)
    vae.requires_grad_(False)
    unet.requires_grad_(False)

    lora_config = LoraConfig(
        r=config.lora_rank,
        lora_alpha=config.lora_alpha,
        init_lora_weights="gaussian",
        target_modules=["to_k", "to_q", "to_v", "to_out.0"],
        lora_dropout=config.lora_dropout,
    )
    unet.add_adapter(lora_config)
    cast_trainable_parameters(unet, torch.float32)

    text_encoder.to(device, dtype=weight_dtype)
    vae.to(device, dtype=weight_dtype)
    unet.to(device)

    text_encoder.eval()
    vae.eval()
    unet.train()

    return noise_scheduler, tokenizer, text_encoder, vae, unet


def save_lora_weights(
    unet: UNet2DConditionModel,
    config: LoraTrainingConfig,
) -> Path:
    """保存 U-Net LoRA 权重。"""
    lora_dir = config.output_dir / "lora_weights"
    lora_dir.mkdir(parents=True, exist_ok=True)
    unet_lora_state_dict = convert_state_dict_to_diffusers(get_peft_model_state_dict(unet))
    StableDiffusionPipeline.save_lora_weights(
        save_directory=lora_dir,
        unet_lora_layers=unet_lora_state_dict,
        weight_name=config.lora_weight_name,
        safe_serialization=True,
    )
    return lora_dir / config.lora_weight_name


def train_lora(
    config: LoraTrainingConfig,
    dataloader_builder: Callable[[CLIPTokenizer], DataLoader[dict[str, torch.Tensor]]],
    train_sample_count: int,
) -> tuple[Path, list[dict[str, float]]]:
    """执行 U-Net LoRA 微调并返回权重路径和训练日志。"""
    device = torch.device("cuda")
    weight_dtype = torch.float16

    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        noise_scheduler, tokenizer, text_encoder, vae, unet = load_training_modules(
            config,
            device,
            weight_dtype,
        )

    dataloader = dataloader_builder(tokenizer)
    lora_parameters = [parameter for parameter in unet.parameters() if parameter.requires_grad]
    if not lora_parameters:
        raise RuntimeError("未找到可训练的 LoRA 参数。")

    optimizer = torch.optim.AdamW(
        lora_parameters,
        lr=config.learning_rate,
        betas=(config.adam_beta1, config.adam_beta2),
        weight_decay=config.adam_weight_decay,
        eps=config.adam_epsilon,
    )
    lr_scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
        optimizer,
        T_max=max(config.max_train_steps, 1),
    )

    total_parameters, trainable_parameters = count_parameters(unet)
    print(f"训练样本数: {train_sample_count}")
    print(f"U-Net 参数量: {total_parameters}")
    print(f"LoRA 可训练参数量: {trainable_parameters}")
    print(f"开始 LoRA 微调: {config.max_train_steps} steps")

    global_step = 0
    accumulation_step = 0
    loss_window: list[float] = []
    train_logs: list[dict[str, float]] = []
    progress_bar = tqdm(total=config.max_train_steps, desc="LoRA 微调")

    while global_step < config.max_train_steps:
        for batch in dataloader:
            pixel_values = batch["pixel_values"].to(device=device, dtype=weight_dtype)
            input_ids = batch["input_ids"].to(device=device)

            with torch.no_grad():
                latents = vae.encode(pixel_values).latent_dist.sample()
                latents = latents * vae.config.scaling_factor
                noise = torch.randn_like(latents)
                timesteps = torch.randint(
                    0,
                    noise_scheduler.config.num_train_timesteps,
                    (latents.shape[0],),
                    device=device,
                    dtype=torch.long,
                )
                noisy_latents = noise_scheduler.add_noise(latents, noise, timesteps)
                encoder_hidden_states = text_encoder(input_ids, return_dict=False)[0]

            model_pred = unet(
                noisy_latents,
                timesteps,
                encoder_hidden_states,
                return_dict=False,
            )[0]

            prediction_type = noise_scheduler.config.prediction_type
            if prediction_type == "epsilon":
                target = noise
            elif prediction_type == "v_prediction":
                target = noise_scheduler.get_velocity(latents, noise, timesteps)
            else:
                raise ValueError(f"不支持的预测类型: {prediction_type}")

            loss = F.mse_loss(model_pred.float(), target.float(), reduction="mean")
            (loss / config.gradient_accumulation_steps).backward()
            accumulation_step += 1
            loss_window.append(float(loss.detach().cpu()))

            if accumulation_step % config.gradient_accumulation_steps == 0:
                torch.nn.utils.clip_grad_norm_(lora_parameters, max_norm=1.0)
                optimizer.step()
                lr_scheduler.step()
                optimizer.zero_grad(set_to_none=True)

                global_step += 1
                current_lr = float(lr_scheduler.get_last_lr()[0])
                mean_loss = float(sum(loss_window) / len(loss_window))
                train_logs.append(
                    {
                        "step": float(global_step),
                        "loss": mean_loss,
                        "learning_rate": current_lr,
                    }
                )
                loss_window.clear()
                progress_bar.update(1)

                if global_step % config.log_interval == 0 or global_step == 1:
                    print(f"step={global_step}, loss={mean_loss:.6f}, lr={current_lr:.8f}")

                if global_step >= config.max_train_steps:
                    break

        if global_step >= config.max_train_steps:
            break

    progress_bar.close()
    lora_path = save_lora_weights(unet, config)

    del noise_scheduler, tokenizer, text_encoder, vae, unet
    torch.cuda.empty_cache()
    return lora_path, train_logs


def generate_sd15_sample(
    config: LoraTrainingConfig,
    prompt: str,
    negative_prompt: str,
    image_path: Path,
    width: int,
    height: int,
    num_inference_steps: int,
    guidance_scale: float,
    lora_path: Path | None = None,
) -> Path:
    """使用 SD1.5 生成样例图，可选加载 LoRA 权重。"""
    image_path.parent.mkdir(parents=True, exist_ok=True)
    diffusers_verbosity = diffusers_logging.get_verbosity()
    diffusers_logging.set_verbosity_error()
    try:
        with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
            pipe = StableDiffusionPipeline.from_pretrained(
                config.model_id,
                torch_dtype=torch.float16,
                variant="fp16",
                local_files_only=True,
                safety_checker=None,
                requires_safety_checker=False,
            )
            pipe.scheduler = DPMSolverMultistepScheduler.from_config(pipe.scheduler.config)
            if lora_path is not None:
                pipe.load_lora_weights(lora_path.parent, weight_name=lora_path.name)
    finally:
        diffusers_logging.set_verbosity(diffusers_verbosity)

    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    pipe = pipe.to("cuda")

    generator = torch.Generator(device="cuda").manual_seed(config.seed)
    with torch.inference_mode():
        image = pipe(
            prompt=prompt,
            negative_prompt=negative_prompt,
            width=width,
            height=height,
            num_inference_steps=num_inference_steps,
            guidance_scale=guidance_scale,
            generator=generator,
        ).images[0]

    image.save(image_path)
    del pipe
    torch.cuda.empty_cache()
    return image_path


def generate_lora_sample(
    config: LoraTrainingConfig,
    lora_path: Path,
    prompt: str,
    negative_prompt: str,
    image_path: Path,
    width: int,
    height: int,
    num_inference_steps: int,
    guidance_scale: float,
) -> Path:
    """加载微调后的 LoRA 权重并生成样例图。"""
    return generate_sd15_sample(
        config=config,
        prompt=prompt,
        negative_prompt=negative_prompt,
        image_path=image_path,
        width=width,
        height=height,
        num_inference_steps=num_inference_steps,
        guidance_scale=guidance_scale,
        lora_path=lora_path,
    )
