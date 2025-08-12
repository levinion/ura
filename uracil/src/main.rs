mod client;

use std::{
    io::{self, Write},
    path::PathBuf,
};

use anyhow::{Result, anyhow};
use clap::Parser;

#[derive(Parser)]
struct Cli {
    #[arg(short = 'c')]
    code: Option<String>,
    path: Option<String>,
}

fn shell_mode() -> Result<()> {
    let client = client::UraIPCClient::new()?;
    loop {
        print!("> ");
        io::stdout().flush()?;
        let mut input = String::new();
        match io::stdin().read_line(&mut input) {
            Ok(bytes_read) => {
                if bytes_read == 0 {
                    break;
                }
                let input = input.trim();
                if input.is_empty() {
                    continue;
                }
                if input == "exit" {
                    break;
                }
                let request = client::UraIPCRequestMessage {
                    method: "execute".to_string(),
                    body: input.to_string(),
                };
                match client.send(&request) {
                    Ok(reply) => reply.print(),
                    Err(err) => eprintln!("{}", err),
                }
            }
            Err(err) => {
                eprintln!("{:?}", err);
                break;
            }
        }
    }
    client.destroy()?;
    Ok(())
}

fn oneshot(code: String) -> Result<()> {
    let client = client::UraIPCClient::new()?;
    let request = client::UraIPCRequestMessage {
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
