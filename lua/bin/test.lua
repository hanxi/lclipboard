local clipboard = require "clipboard"

local function on_cliboard_change(text, from)
    print(text, from)
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
        clipboard.settext("Hello World\n")
        test_from_settext = false
    end
end
