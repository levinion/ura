use vipera::Vipera;

use crate::keymap::KeyMap;

#[derive(serde::Serialize, serde::Deserialize, Debug)]
pub struct Config {
    pub scale: f64,
    pub keymap: KeyMap,
    pub xcursor_size: Option<u32>,
    pub refresh: Option<i32>,
}

impl Config {
    pub fn init() -> Self {
        Vipera::new()
            .set_config_name("config.toml")
            .add_config_path("/etc/ura")
            .add_config_path("$HOME/.config/ura")
            .extract()
            .unwrap_or(Default::default())
    }
}

impl Default for Config {
    fn default() -> Self {
        Self {
            scale: 1.0,
            keymap: KeyMap::default(),
            xcursor_size: None,
            refresh: None,
        }
    }
}
