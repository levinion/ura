use std::fmt::Debug;

pub use smithay::input::keyboard::ModifiersState;
pub use xkbcommon::xkb::Keysym;

use crate::KeyAction;

pub trait Controller: Debug + 'static {
    fn scale_factor(&self) -> f64;

    fn process_keyboard_shortcut(
        &self,
        modifiers: ModifiersState,
        keysym: Keysym,
    ) -> Option<KeyAction>;

    fn xcursor_size(&self) -> u32 {
        std::env::var("XCURSOR_SIZE")
            .ok()
            .and_then(|s| s.parse().ok())
            .unwrap_or(24)
    }

    fn refresh(&self) -> i32 {
        60_000
    }
}
