# Ura completion with lua_ls

## Enhancing Your Neovim Lua Development with `ura` Completion

This guide provides comprehensive instructions on how to set up intelligent code completion for `ura`'s Lua API within Neovim, specifically when using `lua-language-server` as your Language Server Protocol (LSP). By integrating `ura`'s type definition file, you'll gain access to powerful auto-completion, diagnostics, and documentation, significantly boosting your productivity when developing `ura` configurations or scripts.

## Understanding `ura`'s Completion File

`ura` provides a dedicated type definition file that outlines its Lua API. This file is crucial for the language server to understand `ura`'s functions, methods, and global variables.

* **Location of the Completion File:**
  The `ura` completion file is typically located at `/usr/share/lua/5.1/ura/type.lua`. This path is essential as we will instruct `lua-language-server` to include it in its library paths.

## Configuring `lua-language-server` for `ura` Completion

We will use `nvim-lspconfig` and `lazy.nvim` to set up `lua-language-server` with the necessary configurations to recognize `ura`'s API.

### Using `lazy.nvim`

If you are managing your Neovim plugins with `lazy.nvim`, you can integrate the `lua-language-server` configuration directly into your plugin specification.

**Add or modify the following in your `lazy.nvim` configuration (e.g., in a file within `lua/plugins/`):**

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

**Explanation of the Configuration:**

* **`"neovim/nvim-lspconfig"`:** This line specifies that we are configuring `nvim-lspconfig`, which is a common plugin for setting up various LSP servers in Neovim.
* **`config = function()`:** This block contains the configuration logic for `nvim-lspconfig` specifically for `lua-language-server`.
* **`local lspconfig = require("lspconfig")`:** Imports the `lspconfig` module.
* **`local library = { ... }`:** This table defines the paths that `lua-language-server` will scan for Lua libraries and type definitions.

  * `vim.env.VIMRUNTIME`: Includes Neovim's runtime files, providing completion for Neovim's Lua API (`vim.*`).
  * `"/usr/share/lua/5.1/"`: **Crucially, this line adds the directory where `ura`'s `type.lua` file resides.** By including this path, `lua-language-server` will discover and parse `ura/type.lua`, enabling completion for `ura`'s API.
* **`lspconfig.lua_ls.setup { ... }`:** This sets up the `lua-language-server` (aliased as `lua_ls` in `lspconfig`).
* **`settings = { Lua = { ... } }`:** These are the specific settings passed to `lua-language-server`.

  * **`runtime = { version = 'LuaJIT' }`:** Specifies the Lua runtime version. If your `ura` environment uses LuaJIT (which is common for tiling window managers), set this to `'LuaJIT'`. Otherwise, adjust it to your Lua version (e.g., `'5.1'`, `'5.3'`, etc.).
  * **`workspace = { library = library }`:** This tells the language server to consider the paths defined in our `library` table when resolving symbols and providing completion.
  * **`telemetry = { enable = false }`:** Disables telemetry, which is often preferred for privacy or to reduce network traffic.
  * **`diagnostics = { globals = { "vim", "ura" } }`:** This is another vital part. It declares `vim` and `ura` as global variables. Without this, `lua-language-server` might report "undeclared global" warnings for `vim.*` and `ura.*` calls in your configuration files, even if completion works.

## Final Steps and Verification

1. **Ensure `lua-language-server` is installed:**
   If you haven't already, install `lua-language-server`. You can typically do this via your system's package manager or by following the instructions on its GitHub page.
2. **Update Neovim plugins:**
   After modifying your `lazy.nvim` configuration, make sure to sync your plugins by opening Neovim and running `:Lazy sync` or `:Lazy install` if `lazy.nvim` doesn't do it automatically.
3. **Restart Neovim:**
   Close and reopen Neovim to ensure that the LSP server restarts with the new configuration.
4. **Test the completion:**
   Open any Lua file that is part of your `ura` configuration (e.g., `init.lua`). When you type `ura.`, you should now see a list of `ura` functions and methods available for auto-completion. Similarly, typing `ura.` followed by a method (e.g., `ura.ws.`) should provide completion for workspace-related functions.

By following these steps, you'll have a robust development environment for `ura` configurations in Neovim, complete with intelligent code completion, making your workflow smoother and more efficient.
