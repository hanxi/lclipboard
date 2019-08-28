local clipboard = require "clipboard"

function string.tohex(str)
    return (str:gsub('.', function (c)
        return string.format('%02X', string.byte(c))
    end))
end

local function on_cliboard_change(text, from)
    print(text:tohex(), text, from)
    local f = io.open("tmp.txt", "w+")
    f:write(text)
    f:close()
    if text == "exit" then
        clipboard.exit()
    end
end
clipboard.init(on_cliboard_change)

local test_from_settext = true
while true do
    if clipboard.loop() == -1 then
        break
    end

    if test_from_settext then
        clipboard.settext("你好\n")
        test_from_settext = false
    end
end
