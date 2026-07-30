Option Explicit

Dim targetPath, wordApp, document, found
targetPath = WScript.Arguments(0)
found = False

On Error Resume Next
Set wordApp = GetObject(, "Word.Application")
If Err.Number <> 0 Then
    WScript.Echo "WORD_NOT_RUNNING"
    WScript.Quit 0
End If
On Error GoTo 0

For Each document In wordApp.Documents
    If LCase(document.FullName) = LCase(targetPath) Then
        found = True
        If Not document.Saved Then
            WScript.Echo "TARGET_UNSAVED"
            WScript.Quit 3
        End If
        document.Close False
        WScript.Echo "TARGET_CLOSED"
        Exit For
    End If
Next

If Not found Then
    WScript.Echo "TARGET_NOT_OPEN"
End If
