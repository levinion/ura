use std::collections::HashSet;

use rustyline::{
    Helper, Highlighter, Hinter, Validator,
    completion::{Completer, Pair},
    highlight::MatchingBracketHighlighter,
    hint::HistoryHinter,
    validate::MatchingBracketValidator,
};

#[derive(Helper, Hinter, Validator, Highlighter)]
pub struct UracilHelper {
    #[rustyline(Highlighter)]
    highlighter: MatchingBracketHighlighter,
    #[rustyline(Validator)]
    validator: MatchingBracketValidator,
    #[rustyline(Hinter)]
    hinter: HistoryHinter,
    keywords: HashSet<String>,
}

impl UracilHelper {
    pub fn new() -> Self {
        let keywords: HashSet<String> = [
            // buildin,
            ":clear",
            ":quit",
            ":clear_history",
            "ura",
            // ura.api
            "ura.api",
            "ura.api.terminate",
            "ura.api.reload",
            "ura.api.notify_idle_activity",
            "ura.api.set_idle_inhibitor",
            // ura.win
            "ura.win",
            "ura.win.focus",
            "ura.win.close",
            "ura.win.move_to_workspace",
            "ura.win.move_to_workspace_or_create",
            "ura.win.size",
            "ura.win.get_current",
            "ura.win.get",
            "ura.win.activate",
            "ura.win.move",
            "ura.win.resize",
            "ura.win.center",
            "ura.win.set_layout",
            "ura.win.set_z_index",
            "ura.win.set_draggable",
            "ura.win.swap",
            "ura.win.redraw",
            // ura.input
            "ura.input",
            "ura.input.keyboard",
            "ura.input.keyboard.set_repeat",
            "ura.input.cursor",
            "ura.input.cursor.set_theme",
            "ura.input.cursor.set_visible",
            "ura.input.cursor.is_visible",
            "ura.input.cursor.set_shape",
            // ura.ws
            "ura.ws",
            "ura.ws.create",
            "ura.ws.switch",
            "ura.ws.switch_or_create",
            "ura.ws.size",
            "ura.ws.destroy",
            "ura.ws.get_current",
            "ura.ws.get",
            "ura.ws.list",
            "ura.ws.redraw",
            // ura.output
            "ura.output",
            "ura.output.get_current",
            "ura.output.set_dpms",
            // ura.layout
            "ura.layout",
            "ura.layout.set",
            "ura.layout.unset",
            // ura.keymap
            "ura.keymap",
            "ura.keymap.set",
            "ura.keymap.set_mode",
            "ura.keymap.unset",
            "ura.keymap.unset_mode",
            "ura.keymap.enter_mode",
            "ura.keymap.get_current_mode",
            // ura.hook
            "ura.hook",
            "ura.hook.set",
            // ura.fn
            "ura.fn",
            "ura.fn.set_env",
            "ura.fn.unset_env",
            "ura.fn.append_package_path",
            "ura.fn.prepend_package_path",
            "ura.fn.expanduser",
            "ura.fn.expandvars",
            "ura.fn.expand",
            // ura.opt
            "ura.opt",
            "ura.opt.active_border_color",
            "ura.opt.inactive_border_color",
            "ura.opt.border_width",
            "ura.opt.focus_follow_mouse",
            "ura.opt.tilling",
            "ura.opt.tilling.gap",
            "ura.opt.tilling.gap.outer",
            "ura.opt.tilling.gap.outer.top",
            "ura.opt.tilling.gap.outer.left",
            "ura.opt.tilling.gap.outer.bottom",
            "ura.opt.tilling.gap.outer.right",
            "ura.opt.tilling.gap.inner",
        ]
        .iter()
        .map(|&s| s.to_string())
        .collect();
        let highlighter = MatchingBracketHighlighter::new();
        let validator = MatchingBracketValidator::new();
        let hinter = HistoryHinter::new();
        Self {
            highlighter,
            validator,
            hinter,
            keywords,
        }
    }
}

impl Completer for UracilHelper {
    type Candidate = Pair;
    fn complete(
        &self,
        line: &str,
        pos: usize,
        _ctx: &rustyline::Context<'_>,
    ) -> rustyline::Result<(usize, Vec<Self::Candidate>)> {
        let prefix = &line[0..pos];
        let word_start = prefix
            .rfind(|c: char| !c.is_alphanumeric() && c != '.' && c != '_')
            .map_or(0, |idx| idx + 1);
        let word_prefix = &line[word_start..pos];
        let mut matches = vec![];
        for keyword in &self.keywords {
            if keyword.starts_with(word_prefix) {
                matches.push(Pair {
                    display: keyword.clone(),
                    replacement: keyword.clone(),
                });
            }
        }
        Ok((word_start, matches))
    }
}
