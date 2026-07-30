Option Explicit

Dim fso, inputPath, outputPath, pdfPath, wordApp, document, ownsWord, toc
Set fso = CreateObject("Scripting.FileSystemObject")
If WScript.Arguments.Count <> 3 Then
    WScript.Quit 2
End If

inputPath = fso.GetAbsolutePathName(WScript.Arguments(0))
outputPath = fso.GetAbsolutePathName(WScript.Arguments(1))
pdfPath = fso.GetAbsolutePathName(WScript.Arguments(2))
ownsWord = False

On Error Resume Next
Set wordApp = GetObject(, "Word.Application")
If Err.Number <> 0 Then
    Err.Clear
    Set wordApp = CreateObject("Word.Application")
    If Err.Number <> 0 Then
        WScript.Echo "WORD_ERROR " & Err.Number & " " & Err.Description
        WScript.Quit 3
    End If
    ownsWord = True
    wordApp.Visible = False
End If
wordApp.DisplayAlerts = 0

Err.Clear
Set document = wordApp.Documents.Open(inputPath, False, False, False)
If Err.Number <> 0 Then
    WScript.Echo "OPEN_ERROR " & Err.Number & " " & Err.Description
    If ownsWord Then wordApp.Quit
    WScript.Quit 4
End If

On Error GoTo 0
document.Repaginate
document.Fields.Update
For Each toc In document.TablesOfContents
    toc.Update
Next
document.Repaginate
document.Fields.Update
document.SaveAs2 outputPath, 16
document.ExportAsFixedFormat pdfPath, 17
document.Close False
If ownsWord Then wordApp.Quit
WScript.Echo outputPath
