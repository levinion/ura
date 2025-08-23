mod ipc;
mod shell;

use std::path::PathBuf;

use anyhow::{Result, anyhow};
use clap::Parser;

use crate::shell::UracilShell;

#[derive(Parser)]
struct Cli {
    #[arg(short = 'c')]
    code: Option<String>,
    path: Option<String>,
}

fn shell_mode() -> Result<()> {
    let mut shell = UracilShell::new()?;
    shell.run()?;
    Ok(())
}

fn oneshot(code: String) -> Result<()> {
    let client = ipc::UraIPCClient::new()?;
    let request = ipc::UraIPCRequestMessage {
        method: "execute".to_string(),
        body: code,
    };
    let reply = client.send(&request)?;
    reply.print();
    client.destroy()?;
    Ok(())
}

fn oneshot_file(path: String) -> Result<()> {
    let path = PathBuf::from(path);
    if !path.is_file() {
        return Err(anyhow!(
            "the given path `{}` doesn't exist or is not a regular file",
            path.to_string_lossy()
        ));
    }
    let code = std::fs::read_to_string(&path)?;
    oneshot(code)?;
    Ok(())
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.code {
        Some(code) => oneshot(code)?,
        None => match cli.path {
            Some(path) => oneshot_file(path)?,
            None => shell_mode()?,
        },
    }
    Ok(())
}
