# Lua Compeletion

Ura's completion file will be put in /usr/share/lua/5.1/ura/*. If you are using neovim with lua-language-server as your lsp, follow the instrustion below.

## lazy.nvim

```lua
return {
    "neovim/nvim-lspconfig",
    config = function()
        local lspconfig = require("lspconfig")
        local library = {
            vim.env.VIMRUNTIME,
            -- add /usr/share/lua/5.1 to library
            "/usr/share/lua/5.1/",
        }
        lspconfig.lua_ls.setup {
            settings = {
                Lua = {
                    runtime = {
                        version = 'LuaJIT',
                    },
                    workspace = { library = library },
                    telemetry = { enable = false },
                    diagnostics = {
                        -- add ura as a global variable
                        globals = { "vim", "ura" },
                    },
                },
            }
        }
    end
}
```


