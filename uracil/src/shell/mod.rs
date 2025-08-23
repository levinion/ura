mod helper;

use helper::UracilHelper;

use anyhow::Result;
use rustyline::{Editor, config::Configurer, error::ReadlineError, history::DefaultHistory};

use crate::ipc::{UraIPCClient, UraIPCRequestMessage};

pub struct UracilShell {
    editor: Editor<UracilHelper, DefaultHistory>,
    client: UraIPCClient,
    quit: bool,
}

impl UracilShell {
    pub fn new() -> Result<Self> {
        let mut editor = Editor::<UracilHelper, DefaultHistory>::new()?;
        let helper = UracilHelper::new();
        editor.set_helper(Some(helper));
        editor.set_completion_type(rustyline::CompletionType::List);
        let client = UraIPCClient::new()?;
        Ok(Self {
            editor,
            client,
            quit: false,
        })
    }

    pub fn run(&mut self) -> Result<()> {
        loop {
            let readline = self.editor.readline(">> ");
            match readline {
                Ok(line) => {
                    let line = line.trim();
                    self.editor.add_history_entry(line)?;

                    if line.starts_with(":") {
                        self.process_buildin(line)?;
                        if self.quit {
                            break;
                        };
                        continue;
                    }

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

    fn process_buildin(&mut self, line: &str) -> Result<()> {
        match line {
            ":clear" => self.editor.clear_screen()?,
            ":quit" => self.quit = true,
            ":clear_history" => self.editor.clear_history()?,
            _ => println!("invalid buildin command"),
        };
        Ok(())
    }
}
