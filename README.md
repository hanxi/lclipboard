# lclipboard
system cliboard bind for lua

# API

```lua
local clipboard = require "clipboard"

-- callback when clipboard change. from is true when change is from this api settext.
local function on_cliboard_change(text, from)
    print(text, from)
end

-- init clipboard.
clipboard.init(on_cliboard_change)

-- event loop. return -1 means exit.
clipboard.loop()

-- exit
clipboard.exit()
```

# BUILD

See `build.bat`

# TEST

See `lua/bin/test.lua`

# TODO

Now only support windows.
