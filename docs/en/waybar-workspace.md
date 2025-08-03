# Integrate ura Workspaces with Waybar

This guide will walk you through the process of displaying and managing your ura workspaces directly within Waybar, providing a seamless and informative experience. By combining `ura`'s powerful workspace management with Waybar's customizable interface, you can create a dynamic and efficient desktop environment.

## Understanding the Core Components

At the heart of this integration are `ura`'s IPC (Inter-Process Communication) capabilities, which allow us to query workspace information. We'll then use a Python script to format this information for Waybar, and finally, configure Waybar and `ura` to work in harmony.

## Retrieving Workspace Information with `ura` IPC

`ura` provides simple commands to retrieve crucial workspace details. These commands are executed via `uracil`, `ura`'s IPC utility.

* **Get the current workspace index:**
  This command returns the numerical index of your currently active workspace.
  
  ```shell
  uracil -c 'print(ura.ws.get_current().index)'
  ```

* **Get the total number of workspaces:**
  This command provides the total count of active workspaces.
  
  ```shell
  uracil -c 'print(ura.ws.size())'
  ```

## Crafting the Waybar Workspace Script

This Python script acts as an intermediary, fetching data from `ura` and formatting it for Waybar. It displays your workspaces using distinct icons for active and inactive states, giving you a clear visual representation.

**Save this script, for example, as `~/.config/waybar/scripts/workspace.py`:**

```python
#!/usr/bin/python3

import subprocess

def main():
    active = ""
    inactive = ""
    index = int(
        subprocess.check_output(
            "uracil -c 'print(ura.ws.get_current().index)'", shell=True, text=True
        ).strip()
    )
    number = int(
        subprocess.check_output(
            "uracil -c 'print(ura.ws.size())'", shell=True, text=True
        ).strip()
    )
    workspaces = [inactive] * number
    workspaces[index] = active
    print("  ".join(workspaces))

if __name__ == "__main__":
    main()
```

**Explanation:**

* The `active` and `inactive` variables define the icons used to represent the state of each workspace. You can customize these with any suitable Unicode characters or Nerd Font icons.
* The script uses `subprocess.check_output` to execute the `uracil` commands and capture their output.
* It then creates a list of `inactive` icons and sets the icon at the `index` of the current workspace to `active`.
* Finally, it prints the joined string of workspace icons, which Waybar will then display.

---

### Integrating with Waybar's Configuration

Now, let's tell Waybar to use the Python script to display your workspaces. You'll add a `custom/workspace` module to your Waybar `config.jsonc` file.

**Add the following block to your `waybar`'s `config.jsonc`:**

```jsonc
  "custom/workspace": {
    "exec": "~/.config/waybar/scripts/workspace.py",
    "format": "{text}",
    "signal": 8
  }
```

**Key Points:**

* `"exec"`: Specifies the path to your Python script. Ensure the script is executable (`chmod +x ~/.config/waybar/scripts/workspace.py`).
* `"format"`: Defines how the output from your script will be displayed. `{text}` simply uses the raw output from the script.
* `"signal": 8`: This is crucial for real-time updates. It tells Waybar to re-execute the script whenever it receives a `SIGRTMIN+8` signal. We'll use this in the `ura` configuration.

## Enhancing `ura` for Dynamic Waybar Updates

To make the Waybar module responsive to your workspace changes, you need to configure `ura` to send a signal to Waybar whenever you switch or move windows between workspaces. This is achieved by adding `os.execute("pkill -SIGRTMIN+8 waybar &")` to your `ura` keybindings.

**Add or modify the following keybindings in your `ura`'s `init.lua`:**

```lua
  ura.keymap.set("ctrl", "left", function()
    local index = ura.ws.get_current().index()
    ura.ws.switch(index - 1)
    ura.ws.destroy(index)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("ctrl", "right", function()
    local index = ura.ws.get_current().index()
    ura.ws.switch(index + 1)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("ctrl+shift", "left", function()
    local ws = ura.ws.get_current()
    local win = ura.win.get_current()
    if not win then return end
    ura.win.move_to_workspace(win.index, ws.index - 1)
    ura.ws.destroy(ws.index)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
  ura.keymap.set("ctrl+shift", "right", function()
    local ws = ura.ws.get_current()
    local win = ura.win.get_current()
    if not win then return end
    ura.win.move_to_workspace(win.index, ws.index + 1)
    os.execute("pkill -SIGRTMIN+8 waybar &")
  end)
```

**Explanation of the `init.lua` additions:**

* Each `ura.keymap.set` block defines a keyboard shortcut for workspace navigation or window movement.
* `os.execute("pkill -SIGRTMIN+8 waybar &")` is the key line. After `ura` performs a workspace action (switching, destroying, or moving a window), this command sends the `SIGRTMIN+8` signal to all running Waybar instances. This prompts Waybar to refresh the `custom/workspace` module, ensuring your workspace display is always up-to-date.

## Final Steps and Considerations

1. **Make the Python script executable:**
   
   ```bash
   chmod +x ~/.config/waybar/scripts/workspace.py
   ```
2. **Restart Waybar and `ura`:** For the changes to take effect, you'll need to restart both Waybar and your `ura` session.
   * You can typically restart Waybar with `killall waybar && waybar &` or by logging out and back in.
   * Restarting `ura` usually involves logging out of your session and logging back in.

Now, as you navigate your `ura` workspaces, your Waybar should dynamically update to show your current active workspace and the total number of workspaces, providing a sleek and functional overview of your desktop environment.

## Further Customization

* **Icons:** Experiment with different icons in your Python script (`active` and `inactive` variables) to match your aesthetic preferences. Nerd Fonts offer a vast collection of icons.
* **Styling:** You can further style the Waybar module using CSS in your `style.css` file to change colors, fonts, and spacing. The `custom/workspace` module will have the id `#custom-workspace` by default.
