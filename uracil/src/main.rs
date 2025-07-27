use std::{env::args, os::unix::net::UnixDatagram, path::PathBuf, time::Duration};

use anyhow::{Result, anyhow};

#[derive(serde::Serialize)]
struct UraIPCRequestMessage {
    method: String,
    body: String,
}

#[derive(serde::Deserialize)]
struct UraIPCReplyMessage {
    status: String,
    body: String,
}

fn main() -> Result<()> {
    let server_socket_path = PathBuf::from("/tmp/ura-socket");
    if !server_socket_path.exists() {
        return Err(anyhow!("Server socket not exists"));
    }
    let socket_path = format!("/tmp/ura-client-{}.sock", uuid::Uuid::new_v4());
    let socket_path = PathBuf::from(socket_path);
    if socket_path.exists() {
        std::fs::remove_file(&socket_path)?;
    }
    let socket = UnixDatagram::bind(&socket_path)?;
    socket.connect(server_socket_path)?;
    socket.set_read_timeout(Some(Duration::from_secs(5)))?;
    socket.set_write_timeout(Some(Duration::from_secs(5)))?;

    let code = args().nth(1).expect("uracil expects 1 argument");

    let request = UraIPCRequestMessage {
        method: "execute".to_string(),
        body: code,
    };
    socket.send(serde_json::to_string(&request)?.as_bytes())?;
    let mut buf = [0; 4096];
    let result = match socket.recv(&mut buf) {
        Ok(len) => {
            let reply: UraIPCReplyMessage = serde_json::from_slice(&buf[..len])?;
            if reply.status == "success" {
                Ok(reply.body)
            } else {
                Err(reply.body)
            }
        }
        Err(err) => Err(err.to_string()),
    };
    match result {
        Ok(message) => println!("{}", message),
        Err(err) => eprintln!("{}", err),
    }
    std::fs::remove_file(socket_path)?;
    Ok(())
}
