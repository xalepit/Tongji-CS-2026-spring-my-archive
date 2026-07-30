from pathlib import Path
from docx import Document
from lxml import etree
import zipfile

doc = Document(Path(__file__).with_name("final.docx"))
names = {"Heading 1", "标题 21", "报告正文", "报告参考文献", "toc 1", "toc 2"}
for style in doc.styles:
    if style.name in names:
        print(style.name)
        print(etree.tostring(style.element, encoding="unicode", pretty_print=True))
with zipfile.ZipFile(Path(__file__).with_name("final.docx")) as archive:
    root = etree.fromstring(archive.read("word/styles.xml"))
    print("DOC_DEFAULTS")
    print(etree.tostring(root[0], encoding="unicode", pretty_print=True))
