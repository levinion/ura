use std::collections::HashMap;

use anyhow::{anyhow, Context, Result};
use ura_core::{
    controller::{Controller, Keysym},
    KeyAction,
};

use crate::cfg::Config;

#[derive(serde::Serialize, serde::Deserialize, Debug)]
pub struct KeyMap(pub HashMap<String, Vec<String>>);

macro_rules! shortcut {
    ($($key:expr),*) => {
        vec![$($key.to_string(),)*]
};
}

impl Default for KeyMap {
    fn default() -> Self {
        let mut keymap = HashMap::new();
        keymap.insert("run alacritty".to_string(), shortcut!("mod", "t"));
        #[rustfmt::skip]
        keymap.insert("run google-chrome-stable".to_string(), shortcut!("mod", "w"));
        keymap.insert("quit".to_string(), shortcut!("mod", "q"));
        Self(keymap)
    }
}

trait IntoKeyAction {
    fn into_key_action(self) -> Result<KeyAction>;
}

impl IntoKeyAction for &str {
    fn into_key_action(self) -> Result<KeyAction> {
        let lack_of_args_error = "Lack of Args";
        let mut parts = self.trim().split_whitespace();
        let name = parts.next().unwrap();
        let action = match name {
            "run" => KeyAction::Run(parts.next().context(lack_of_args_error)?.into()),
            "quit" => KeyAction::Quit,
            "toggle_decoration" => KeyAction::ToggleDecorations,
            "toggle_preview" => KeyAction::TogglePreview,
            "toggle_tint" => KeyAction::ToggleTint,
            "vt_switch" => KeyAction::VtSwitch(parts.next().context(lack_of_args_error)?.parse()?),
            "screen" => KeyAction::Screen(parts.next().context(lack_of_args_error)?.parse()?),
            unknown_name => return Err(anyhow!("Unknow Name Found: {}", unknown_name)),
        };
        Ok(action)
    }
}

impl Controller for Config {
    fn scale_factor(&self) -> f64 {
        self.scale
    }

    fn process_keyboard_shortcut(
        &self,
        modifiers: ura_core::focus::ModifiersState,
        keysym: ura_core::controller::Keysym,
    ) -> Option<KeyAction> {
        for (raw, keys) in &self.keymap.0 {
            let mut flag = true;
            for key in keys {
                flag = match key as &str {
                    "alt" => modifiers.alt,
                    "mod" => modifiers.logo,
                    "ctrl" => modifiers.ctrl,
                    "shift" => modifiers.shift,
                    "return" => keysym == Keysym::Return,
                    "a" => keysym == Keysym::a,
                    "b" => keysym == Keysym::b,
                    "c" => keysym == Keysym::c,
                    "d" => keysym == Keysym::d,
                    "e" => keysym == Keysym::e,
                    "f" => keysym == Keysym::f,
                    "g" => keysym == Keysym::g,
                    "h" => keysym == Keysym::h,
                    "i" => keysym == Keysym::i,
                    "j" => keysym == Keysym::j,
                    "k" => keysym == Keysym::k,
                    "l" => keysym == Keysym::l,
                    "m" => keysym == Keysym::m,
                    "n" => keysym == Keysym::n,
                    "o" => keysym == Keysym::o,
                    "p" => keysym == Keysym::p,
                    "q" => keysym == Keysym::q,
                    "r" => keysym == Keysym::r,
                    "s" => keysym == Keysym::s,
                    "t" => keysym == Keysym::t,
                    "u" => keysym == Keysym::u,
                    "v" => keysym == Keysym::v,
                    "w" => keysym == Keysym::w,
                    "x" => keysym == Keysym::x,
                    "y" => keysym == Keysym::y,
                    "z" => keysym == Keysym::z,
                    "1" => keysym == Keysym::_1,
                    "2" => keysym == Keysym::_2,
                    "3" => keysym == Keysym::_3,
                    "4" => keysym == Keysym::_4,
                    "5" => keysym == Keysym::_5,
                    "6" => keysym == Keysym::_6,
                    "7" => keysym == Keysym::_7,
                    "8" => keysym == Keysym::_8,
                    "9" => keysym == Keysym::_9,
                    "0" => keysym == Keysym::_0,
                    _ => unreachable!(),
                };

                if flag == false {
                    break;
                }
            }
            if flag == true {
                return raw.into_key_action().ok();
            }
        }
        None
    }

    fn xcursor_size(&self) -> u32 {
        let size = std::env::var("XCURSOR_SIZE")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(24);
        self.xcursor_size.unwrap_or(size)
    }

    fn refresh(&self) -> i32 {
        self.refresh.unwrap_or(60_000)
    }
}

#[cfg(test)]
mod test {
    use ura_core::KeyAction;

    use crate::keymap::IntoKeyAction;

    #[test]
    fn test_into_key_action() {
        assert_eq!(
            "run alacritty".into_key_action().unwrap(),
            KeyAction::Run("alacritty".to_string())
        );
        assert_eq!("quit".into_key_action().unwrap(), KeyAction::Quit);
    }
}
