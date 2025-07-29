use anyhow::{Result, anyhow};
use std::{os::unix::net::UnixDatagram, path::PathBuf, time::Duration};

#[derive(serde::Serialize)]
pub struct UraIPCRequestMessage {
    pub method: String,
    pub body: String,
}

#[derive(serde::Deserialize)]
pub struct UraIPCReplyMessage {
    pub status: String,
    pub body: String,
}

pub struct UraIPCClient {
    socket: UnixDatagram,
    socket_path: PathBuf,
}

impl UraIPCClient {
    pub fn new() -> Result<UraIPCClient> {
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
        Ok(UraIPCClient {
            socket,
            socket_path,
        })
    }

    pub fn send(&self, request: &UraIPCRequestMessage) -> Result<UraIPCReplyMessage> {
        self.socket
            .send(serde_json::to_string(&request)?.as_bytes())?;
        let mut buf = [0; 4096];
        match self.socket.recv(&mut buf) {
            Ok(len) => {
                let reply: UraIPCReplyMessage = serde_json::from_slice(&buf[..len])?;
                Ok(reply)
            }
            Err(err) => Err(anyhow!(err)),
        }
    }

    pub fn destroy(&self) -> Result<()> {
        std::fs::remove_file(&self.socket_path)?;
        Ok(())
    }
}

impl UraIPCReplyMessage {
    pub fn print(&self) {
        match self.status.as_str() {
            "success" => println!("{}", self.body.trim()),
            "fail" => eprintln!("{}", self.body.trim()),
            _ => unreachable!(),
        }
    }
}
