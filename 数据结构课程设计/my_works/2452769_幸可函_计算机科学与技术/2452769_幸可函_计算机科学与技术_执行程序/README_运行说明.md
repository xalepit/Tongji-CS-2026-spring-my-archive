# DNA 序列片段错配容错检索引擎

## 软件名称

DNA 序列片段错配容错检索引擎（DNA Search Engine）。

## 开发环境

- C++ 与 Qt 6 Widgets
- Qt 6.11.1 MinGW 64-bit
- CMake 与 Ninja
- Windows 10/11 64-bit

## 启动方式

双击本目录中的 `DNA_SearchEngine.exe`。本目录已包含 Qt 和 MinGW 运行依赖，
无需安装 Qt Creator、Visual Studio 或单独配置环境变量。

## 参数说明

- Reference 长度：2000～5000 bp，默认 3000 bp
- K-mer 长度 K：2～10，默认 6
- 允许错配数 k：0～2，默认 2
- Read 长度：20～150 bp，默认 30 bp
- Reads 数量：1～50，默认 50
- 同时满足：K 不大于 Read 长度，Read 长度不大于 Reference 长度

## 建议测试流程

1. 生成或导入 Reference。
2. 根据当前 Reference 随机生成 Reads，或导入 Reads 文件。
3. 单独点击“建立索引”；Reference 或 K 改变后应重新建立。
4. 在 Reads 列表选择一条 Read 后执行单条检索，或执行高通量批量检索。
5. 在全局概览、局部序列沙盘、检索结果和精确比对详情中核对位置与错配。
6. 使用“导出结果”保存 CSV。

## 输入文件格式

- Reference：支持纯文本或 FASTA；序列可以分行。
- Reads：支持每行一条的纯文本或 FASTA。
- 碱基字符仅允许 A、T、C、G；小写字符会按大写处理。
- `test_reference.fasta` 与 `test_reads.txt` 是导入功能测试样例。

## 输出结果说明

GUI 展示每条 Read 的全部合法匹配位置、汉明距离、独立错配位置、
碱基替换关系、稳定轨道布局及逐碱基对齐详情。导出功能生成带 UTF-8
BOM 的 CSV 结果文件，表头全部使用中文，检索耗时以微秒为单位并保留
两位小数。