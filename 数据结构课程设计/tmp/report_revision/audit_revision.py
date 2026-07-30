import json
import re
import zipfile
from pathlib import Path

from docx import Document
from lxml import etree


path = Path(__file__).with_name("final.docx")
doc = Document(path)
ns = {"w": "http://schemas.openxmlformats.org/wordprocessingml/2006/main"}
report = {"errors": [], "warnings": []}

all_text = "\n".join(p.text for p in doc.paragraphs)
for table in doc.tables:
    for row in table.rows:
        all_text += "\n" + "\t".join(cell.text for cell in row.cells)

for forbidden in (
    "11 系统测试与异常处理",
    "AddressSanitizer",
    "ASan",
    "Windows Sandbox",
    "Application Verifier",
    "100 轮",
    "6412",
):
    if forbidden in all_text:
        report["errors"].append(f"Forbidden text remains: {forbidden}")


def outline_level(paragraph):
    ppr = paragraph.style.element.find("w:pPr", namespaces=ns)
    if ppr is None:
        return None
    level = ppr.find("w:outlineLvl", namespaces=ns)
    return level.get(f"{{{ns['w']}}}val") if level is not None else None


headings = [
    (p.style.name, p.text.strip())
    for p in doc.paragraphs
    if outline_level(p) is not None and p.text.strip()
]
report["headings"] = headings
expected_h1 = [
    "1 课程设计题目与项目概述",
    "2 项目背景与相关研究",
    "3 软件功能与需求分析",
    "4 总体设计思想",
    "5 系统逻辑结构与物理结构",
    "6 核心数据结构设计",
    "7 核心算法设计与复杂度分析",
    "8 系统界面与交互设计",
    "9 系统实现与开发平台",
    "10 系统运行结果及分析",
    "11 系统安装、运行与操作说明",
    "12 个人思考与进一步改进",
    "13 总结",
    "参考文献",
]
actual_h1 = [
    p.text.strip()
    for p in doc.paragraphs
    if outline_level(p) == "0"
]
report["h1_continuous"] = actual_h1 == expected_h1
if actual_h1 != expected_h1:
    report["errors"].append("Heading 1 sequence is not continuous.")

expected_chapter_10 = [
    "10.1 实际运行与数据来源：待补充",
    "10.2 默认参数检索结果分析：待补充",
    "10.3 K-mer 索引耗时与哈希负载对比实验：待补充",
    "10.4 边界情况与异常处理",
]
actual_chapter_10 = [
    p.text.strip()
    for p in doc.paragraphs
    if p.text.strip().startswith("10.")
    and outline_level(p) == "1"
]
report["chapter_10_headings"] = actual_chapter_10
if actual_chapter_10 != expected_chapter_10:
    report["errors"].append("Chapter 10 subsection structure is invalid.")

toc_paragraphs = [
    p.text.strip()
    for p in doc.paragraphs
    if p.style.name.lower().startswith("toc") and p.text.strip()
]
report["toc_entry_count"] = len(toc_paragraphs)
report["toc_first_entries"] = toc_paragraphs[:8]
report["toc_last_entries"] = toc_paragraphs[-8:]
if len(toc_paragraphs) < 40:
    report["errors"].append("Automatic TOC did not populate enough entries.")

with zipfile.ZipFile(path) as archive:
    xml_parts = {
        name: archive.read(name)
        for name in archive.namelist()
        if name.endswith(".xml")
    }
    document_root = etree.fromstring(xml_parts["word/document.xml"])
    styles_root = etree.fromstring(xml_parts["word/styles.xml"])
    default_fonts = styles_root.find(
        ".//w:docDefaults/w:rPrDefault/w:rPr/w:rFonts", namespaces=ns
    )
    report["default_fonts"] = {
        "ascii": default_fonts.get(f"{{{ns['w']}}}ascii") if default_fonts is not None else None,
        "hAnsi": default_fonts.get(f"{{{ns['w']}}}hAnsi") if default_fonts is not None else None,
        "eastAsia": default_fonts.get(f"{{{ns['w']}}}eastAsia") if default_fonts is not None else None,
    }
    report["vertical_tab_count"] = sum(data.count(b"\x0b") for data in xml_parts.values())
    report["soft_break_count"] = len(
        document_root.xpath(".//w:br[not(@w:type) or @w:type='textWrapping']", namespaces=ns)
    )
    fields = document_root.xpath(".//w:instrText/text()", namespaces=ns)
    report["toc_field_present"] = any("TOC" in field for field in fields)

if report["vertical_tab_count"]:
    report["errors"].append("Vertical tab characters remain.")
if report["soft_break_count"]:
    report["errors"].append("Soft/manual line breaks remain.")
if not report["toc_field_present"]:
    report["errors"].append("Automatic TOC field is missing.")
if report["default_fonts"] != {
    "ascii": "Times New Roman",
    "hAnsi": "Times New Roman",
    "eastAsia": "宋体",
}:
    report["errors"].append("Document default Latin or East Asian fonts are inconsistent.")

style_checks = []
for paragraph in doc.paragraphs:
    text = paragraph.text.strip()
    if text == "1 课程设计题目与项目概述":
        expected_size = 16
    elif text == "1.1 课程设计题目":
        expected_size = 12
    elif paragraph.style.name == "报告正文":
        expected_size = 10.5
    elif paragraph.style.name == "报告参考文献":
        expected_size = 10.5
    else:
        continue
    style = paragraph.style
    rpr = style.element.get_or_add_rPr()
    size = style.font.size.pt if style.font.size is not None else None
    ok = size == expected_size
    style_checks.append(
        {
            "style": style.name,
            "expected_size": expected_size,
            "actual_size": size,
            "ok": ok,
        }
    )
deduplicated_checks = {item["style"]: item for item in style_checks}
report["style_checks"] = list(deduplicated_checks.values())
if not all(item["ok"] for item in deduplicated_checks.values()):
    report["errors"].append("One or more report styles use inconsistent fonts or sizes.")

citations = {}
for number in range(1, 5):
    label = f"[{number}]"
    total = superscript = 0
    for p in doc.paragraphs:
        for run in p.runs:
            if label in run.text:
                total += run.text.count(label)
                vert = run._r.find("w:rPr/w:vertAlign", namespaces=ns)
                if vert is not None and vert.get(f"{{{ns['w']}}}val") == "superscript":
                    superscript += run.text.count(label)
    citations[label] = {"total": total, "superscript": superscript}
report["citations"] = citations
if any(item["superscript"] < 1 for item in citations.values()):
    report["errors"].append("One or more references lack a superscript body citation.")

report["table_count"] = len(doc.tables)
report["passed"] = not report["errors"]
print(json.dumps(report, ensure_ascii=False, indent=2))
