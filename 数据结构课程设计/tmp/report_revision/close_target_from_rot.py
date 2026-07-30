import sys

import pythoncom


target = sys.argv[1].casefold()
rot = pythoncom.GetRunningObjectTable()
context = pythoncom.CreateBindCtx(0)
enumerator = rot.EnumRunning()

while True:
    monikers = enumerator.Next(1)
    if not monikers:
        break
    moniker = monikers[0]
    try:
        name = moniker.GetDisplayName(context, None)
    except pythoncom.com_error:
        continue
    if name.casefold() != target:
        continue
    document = rot.GetObject(moniker)
    if not document.Saved:
        print("TARGET_UNSAVED")
        raise SystemExit(3)
    document.Close(False)
    print("TARGET_CLOSED")
    raise SystemExit(0)

print("TARGET_NOT_FOUND")
