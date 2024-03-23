use cfg::Config;

mod cfg;
mod keymap;

fn main() {
    let cfg = Config::init();
    ura_core::run(cfg);
}
