use anyhow::{Result, anyhow, bail};
use clap::{CommandFactory, Parser};
use std::{
    io::{IsTerminal, Read},
    path::PathBuf,
    process::exit,
};

#[allow(clippy::single_component_path_imports)]
use wayland_client;
use wayland_client::{Connection, Dispatch, EventQueue, QueueHandle, protocol::wl_registry};

wayland_scanner::generate_interfaces!("../../protocols/ura-ipc.xml");
wayland_scanner::generate_client_code!("../../protocols/ura-ipc.xml");

#[derive(Parser)]
struct Cli {
    #[arg(short = 'c')]
    code: Option<String>,
    path: Option<String>,
    #[clap(trailing_var_arg = true, allow_hyphen_values = true)]
    args: Vec<String>,
}

struct Client {
    state: State,
    event_queue: EventQueue<State>,
}

impl Client {
    fn new() -> Result<Self> {
        let conn = Connection::connect_to_env().unwrap();
        let mut event_queue = conn.new_event_queue();
        let qh = event_queue.handle();

        let mut state = State { ura_ipc: None };
        let _registry = conn.display().get_registry(&qh, ());
        event_queue.roundtrip(&mut state).unwrap();

        if state.ura_ipc.is_none() {
            bail!("ura-ipc-protocol is not supported by conpositor");
        }

        Ok(Self { state, event_queue })
    }

    fn execute(&self, code: String, args: Vec<String>) {
        let args = args
            .into_iter()
            .enumerate()
            .map(|(i, arg)| format!("arg[{}] = '{}'\n", i + 1, arg))
            .collect::<String>();
        let args = "arg = {}\narg[0] = \"ura-shell\"".to_string() + &args;
        let script = args + &code;
        self.state.ura_ipc.as_ref().unwrap().call(script);
    }

    fn execute_file(&self, path: String, args: Vec<String>) -> Result<()> {
        let path = PathBuf::from(path);
        if !path.is_file() {
            return Err(anyhow!("invalid path"));
        }
        let code = std::fs::read_to_string(&path)?;
        self.execute(code, args);
        Ok(())
    }

    fn dispatch(&mut self) -> Result<()> {
        self.event_queue.blocking_dispatch(&mut self.state)?;
        Ok(())
    }

    fn run(mut self) -> Result<()> {
        loop {
            self.dispatch()?;
        }
    }
}

struct State {
    ura_ipc: Option<ura_ipc::UraIpc>,
}

fn inline_args(path: Option<String>, args: Vec<String>) -> Vec<String> {
    path.into_iter().chain(args).collect()
}

impl Dispatch<wl_registry::WlRegistry, ()> for State {
    fn event(
        state: &mut Self,
        registry: &wl_registry::WlRegistry,
        event: wl_registry::Event,
        _: &(),
        _: &Connection,
        qh: &QueueHandle<Self>,
    ) {
        if let wl_registry::Event::Global {
            name,
            interface,
            version,
        } = event
            && interface == "ura_ipc"
        {
            state.ura_ipc = Some(registry.bind::<ura_ipc::UraIpc, _, _>(name, version, qh, ()));
        }
    }
}

impl Dispatch<ura_ipc::UraIpc, ()> for State {
    fn event(
        _: &mut Self,
        _: &ura_ipc::UraIpc,
        event: ura_ipc::Event,
        _: &(),
        _: &Connection,
        _: &QueueHandle<Self>,
    ) {
        let ura_ipc::Event::Reply { status, msg } = event;
        println!("{}", msg);
        exit(status)
    }
}

fn main() -> Result<()> {
    let mut cmd = Cli::command();
    let cli = Cli::parse();

    match cli.code {
        Some(code) => {
            let client = Client::new()?;
            client.execute(code, inline_args(cli.path, cli.args));
            client.run()
        }
        None => match cli.path {
            Some(path) => {
                let client = Client::new()?;
                client.execute_file(path, cli.args)?;
                client.run()
            }
            None => {
                if std::io::stdin().is_terminal() {
                    cmd.print_help()?;
                    Ok(())
                } else {
                    // piped
                    let mut buf = String::new();
                    std::io::stdin().read_to_string(&mut buf)?;
                    let client = Client::new()?;
                    client.execute(buf, cli.args);
                    client.run()
                }
            }
        },
    }
}

#[cfg(test)]
mod tests {
    use super::{Cli, inline_args};
    use clap::Parser;

    #[test]
    fn parses_inline_code() {
        let cli = Cli::try_parse_from(["ura-shell", "-c", "print('ok')"]).unwrap();
        assert_eq!(cli.code.as_deref(), Some("print('ok')"));
        assert!(cli.path.is_none());
        assert!(cli.args.is_empty());
    }

    #[test]
    fn collects_inline_arguments() {
        let cli = Cli::try_parse_from(["ura-shell", "-c", "print(arg[1])", "alpha", "--literal"])
            .unwrap();
        assert_eq!(inline_args(cli.path, cli.args), ["alpha", "--literal"]);
    }

    #[test]
    fn parses_script_and_arguments() {
        let cli = Cli::try_parse_from(["ura-shell", "config.lua", "alpha", "--literal"]).unwrap();
        assert_eq!(cli.path.as_deref(), Some("config.lua"));
        assert_eq!(cli.args, ["alpha", "--literal"]);
    }

    #[test]
    fn no_arguments_selects_stdin_mode() {
        let cli = Cli::try_parse_from(["ura-shell"]).unwrap();
        assert!(cli.code.is_none());
        assert!(cli.path.is_none());
        assert!(cli.args.is_empty());
    }
}
