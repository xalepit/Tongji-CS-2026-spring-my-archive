Option Explicit

Dim targetPath, document
targetPath = WScript.Arguments(0)

On Error Resume Next
Set document = GetObject(targetPath)
If Err.Number <> 0 Then
    WScript.Echo "TARGET_GET_FAILED " & Err.Number
    WScript.Quit 2
End If
On Error GoTo 0

If Not document.Saved Then
    WScript.Echo "TARGET_UNSAVED"
    WScript.Quit 3
End If

document.Close False
WScript.Echo "TARGET_CLOSED"
