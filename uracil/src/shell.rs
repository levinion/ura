use std::collections::HashSet;

use anyhow::Result;
use rustyline::{
    Editor, Helper, Highlighter, Hinter, Validator,
    completion::{Completer, Pair},
    config::Configurer,
    error::ReadlineError,
    highlight::MatchingBracketHighlighter,
    hint::HistoryHinter,
    history::DefaultHistory,
    validate::MatchingBracketValidator,
};

use crate::ipc::{UraIPCClient, UraIPCRequestMessage};

pub struct UracilShell {
    rl: Editor<UracilHelper, DefaultHistory>,
    client: UraIPCClient,
}

impl UracilShell {
    pub fn new() -> Result<Self> {
        let mut rl = Editor::<UracilHelper, DefaultHistory>::new()?;
        let helper = UracilHelper::new();
        rl.set_helper(Some(helper));
        rl.set_completion_type(rustyline::CompletionType::List);
        let client = UraIPCClient::new()?;
        Ok(Self { rl, client })
    }

    pub fn run(&mut self) -> Result<()> {
        loop {
            let readline = self.rl.readline(">> ");
            match readline {
                Ok(line) => {
                    let line = line.trim();
                    self.rl.add_history_entry(line)?;

                    let line_p = format!("print({})", line);

                    let mut request = UraIPCRequestMessage {
                        method: "execute".to_string(),
                        body: line_p,
                    };
                    match self.client.send(&request) {
                        Ok(reply) => {
                            if reply.success() {
                                reply.print();
                            } else {
                                request.body = line.to_string();
                                match self.client.send(&request) {
                                    Ok(reply) => reply.print(),
                                    Err(err) => eprintln!("{}", err),
                                }
                            }
                        }
                        Err(err) => {
                            println!("Error: {:?}", err);
                        }
                    }
                }
                Err(ReadlineError::Interrupted) | Err(ReadlineError::Eof) => {
                    break;
                }
                Err(err) => {
                    println!("Error: {:?}", err);
                    break;
                }
            }
        }
        self.client.destroy()?;
        Ok(())
    }
}

#[derive(Helper, Hinter, Validator, Highlighter)]
struct UracilHelper {
    #[rustyline(Highlighter)]
    highlighter: MatchingBracketHighlighter,
    #[rustyline(Validator)]
    validator: MatchingBracketValidator,
    #[rustyline(Hinter)]
    hinter: HistoryHinter,
    keywords: HashSet<String>,
}

impl UracilHelper {
    fn new() -> Self {
        let keywords: HashSet<String> = [
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
            "ura.ws.switch",
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
        let mut matches = vec![];
        for keyword in &self.keywords {
            if keyword.starts_with(prefix) {
                matches.push(Pair {
                    display: keyword.clone(),
                    replacement: keyword.clone(),
                });
            }
        }
        Ok((0, matches))
    }
}
