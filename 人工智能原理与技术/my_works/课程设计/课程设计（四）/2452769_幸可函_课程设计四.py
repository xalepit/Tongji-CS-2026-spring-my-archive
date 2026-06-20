# -*- coding: utf-8 -*-
"""
课程设计四 第一小问：复现 Stable Diffusion v1.5 文生图流程。

任务目标：
1. 使用 PDF 指定的 stable-diffusion-v1-5/stable-diffusion-v1-5 权重。
2. 输入古诗“孤舟蓑笠翁，独钓寒江雪”，生成水墨画风格图片。
3. 保存生成结果和本次运行参数。
"""

from __future__ import annotations

import io
import sys
from contextlib import redirect_stderr, redirect_stdout
from datetime import datetime
from pathlib import Path

import torch
from diffusers import DPMSolverMultistepScheduler, StableDiffusionPipeline

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

POEM = "孤舟蓑笠翁，独钓寒江雪"
SEED = DEFAULT_SEED


def build_prompt(poem: str) -> tuple[str, str]:
    """根据古诗内容构造文生图提示词和反向提示词。"""
    prompt = (
        f"Chinese poem: {poem}. "
        "Chinese ink wash painting, xuan paper, black ink, ink diffusion, "
        "brush strokes, blank space, lone fisherman in a small boat, "
        "snowy river, misty mountains"
    )
    negative_prompt = (
        "low quality, blurry, photorealistic, colorful, cartoon, oil painting, "
        "modern city, car, crowd, text, watermark, signature, frame"
    )
    return prompt, negative_prompt


def main() -> None:
    """加载 Stable Diffusion v1.5 并生成古诗水墨画。"""
    set_random_seed(SEED)
    output_dir = make_output_dir(__file__, "question1")

    device = "cuda" if torch.cuda.is_available() else "cpu"
    dtype = torch.float16 if device == "cuda" else torch.float32

    prompt, negative_prompt = build_prompt(POEM)

    # 只使用本机缓存，确保加载的是预备工作中下载好的 PDF 指定 SD v1.5 权重。
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        pipe = StableDiffusionPipeline.from_pretrained(
            MODEL_ID,
            torch_dtype=dtype,
            variant="fp16" if device == "cuda" else None,
            local_files_only=True,
        )
    pipe.scheduler = DPMSolverMultistepScheduler.from_config(pipe.scheduler.config)
    pipe.enable_attention_slicing()
    pipe.set_progress_bar_config(disable=True)
    pipe = pipe.to(device)

    prompt_token_count = len(pipe.tokenizer(prompt, truncation=False).input_ids)
    negative_token_count = len(pipe.tokenizer(negative_prompt, truncation=False).input_ids)
    max_token_count = pipe.tokenizer.model_max_length
    if prompt_token_count > max_token_count:
        raise ValueError("提示词超过 CLIP 最大长度限制，会导致内容被截断。")
    if negative_token_count > max_token_count:
        raise ValueError("反向提示词超过 CLIP 最大长度限制，会导致内容被截断。")

    generator = torch.Generator(device=device).manual_seed(SEED)
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        image = pipe(
            prompt=prompt,
            negative_prompt=negative_prompt,
            width=512,
            height=512,
            num_inference_steps=30,
            guidance_scale=7.5,
            generator=generator,
        ).images[0]

    image_path = output_dir / "q1_poem_ink_fishing_snow.png"
    metadata_path = output_dir / "q1_run_metadata.json"
    image.save(image_path)

    metadata = {
        "task": "课程设计四 第一小问 Stable Diffusion v1.5 文生图复现",
        "time": datetime.now().isoformat(timespec="seconds"),
        "model_id": MODEL_ID,
        "poem": POEM,
        "prompt": prompt,
        "negative_prompt": negative_prompt,
        "prompt_token_count": prompt_token_count,
        "negative_prompt_token_count": negative_token_count,
        "max_token_count": max_token_count,
        "seed": SEED,
        "width": 512,
        "height": 512,
        "num_inference_steps": 30,
        "guidance_scale": 7.5,
        "device": device,
        "dtype": str(dtype),
        **get_runtime_environment(),
        "output_image": str(image_path),
    }
    write_json(metadata_path, metadata)

    print("Stable Diffusion v1.5 文生图复现实验完成")
    print(f"模型权重: {MODEL_ID}")
    print(f"输入诗句: {POEM}")
    print(f"运行设备: {device}")
    if torch.cuda.is_available():
        print(f"GPU: {torch.cuda.get_device_name(0)}")
    print(f"提示词长度: {prompt_token_count}/{max_token_count}")
    print(f"输出图片: {image_path}")
    print(f"运行记录: {metadata_path}")


if __name__ == "__main__":
    main()
