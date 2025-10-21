mod ipc;

use std::path::PathBuf;

use anyhow::{Result, anyhow};
use clap::{CommandFactory, Parser};

#[derive(Parser)]
struct Cli {
    #[arg(short = 'c')]
    code: Option<String>,
    path: Option<String>,
    #[clap(trailing_var_arg = true, allow_hyphen_values = true)]
    args: Vec<String>,
}

fn oneshot(code: String, args: Vec<String>) -> Result<()> {
    let args = args
        .into_iter()
        .enumerate()
        .map(|(i, arg)| format!("arg[{}] = '{}'\n", i + 1, arg))
        .collect::<String>();
    let args = "arg = {}\narg[0] = \"uracil\"".to_string() + &args;
    let client = ipc::UraIPCClient::new()?;
    let request = ipc::UraIPCRequestMessage {
        method: "execute".to_string(),
        body: format!("{}\n{}", args, code),
    };
    let reply = client.send(&request)?;
    reply.print();
    client.destroy()?;
    Ok(())
}

fn oneshot_file(path: String, args: Vec<String>) -> Result<()> {
    let path = PathBuf::from(path);
    if !path.is_file() {
        return Err(anyhow!(
            "the given path `{}` doesn't exist or is not a regular file",
            path.to_string_lossy()
        ));
    }
    let code = std::fs::read_to_string(&path)?;
    oneshot(code, args)?;
    Ok(())
}

fn main() -> Result<()> {
    let mut cmd = Cli::command();
    let cli = Cli::parse();
    match cli.code {
        Some(code) => oneshot(code, cli.args)?,
        None => match cli.path {
            Some(path) => oneshot_file(path, cli.args)?,
            None => {
                cmd.print_help()?;
            }
        },
    }
    Ok(())
}
