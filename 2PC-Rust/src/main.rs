#[macro_use]
extern crate log;
extern crate stderrlog;
extern crate clap;
extern crate ctrlc;
extern crate ipc_channel;
use std::env;
use std::fs;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::process::{Child,Command};
use ipc_channel::ipc::IpcSender as Sender;
use ipc_channel::ipc::IpcReceiver as Receiver;
use ipc_channel::ipc::IpcOneShotServer;
use ipc_channel::ipc::channel;
pub mod message;
pub mod oplog;
pub mod coordinator;
pub mod participant;
pub mod client;
pub mod checker;
pub mod tpcoptions;
use message::MessageType;
use message::ProtocolMessage;
use coordinator::Coordinator;
use participant::Participant;

use crate::client::Client;

///
/// pub fn spawn_child_and_connect(child_opts: &mut tpcoptions::TPCOptions) -> (std::process::Child, Sender<ProtocolMessage>, Receiver<ProtocolMessage>)
///
///     child_opts: CLI options for child process
///
/// 1. Set up IPC
/// 2. Spawn a child process using the child CLI options
/// 3. Do any required communication to set up the parent / child communication channels
/// 4. Return the child process handle and the communication channels for the parent
///
/// HINT: You can change the signature of the function if necessary
///
fn spawn_child_and_connect(child_opts: &mut tpcoptions::TPCOptions, transmit: &mut Sender<ProtocolMessage>) -> (Child, Sender<ProtocolMessage>) {
    // set up one shot
    let (server, serverName): (IpcOneShotServer<(String, Sender<ProtocolMessage>)>, String) = IpcOneShotServer::new().unwrap();
    child_opts.ipc_path = serverName;

    let child = Command::new(env::current_exe().unwrap())
        .args(child_opts.as_vec())
        .spawn()
        .expect("Failed to execute child process");

    // let (tx, rx) = channel().unwrap();
    // let (tx0, rx0): (Sender<ProtocolMessage>, Receiver<ProtocolMessage>) = channel().unwrap();
    // Receive tuple (with 1 shot server name and Sender)
    let (_, receivedVal) = server.accept().unwrap();
       
    // Connect to the server and sent it a transmitter. 
    let temp = Sender::connect(receivedVal.0).unwrap();
    temp.send(transmit);

    (child, receivedVal.1)
}

///
/// pub fn connect_to_coordinator(opts: &tpcoptions::TPCOptions) -> (Sender<ProtocolMessage>, Receiver<ProtocolMessage>)
///
///     opts: CLI options for this process
///
/// 1. Connect to the parent via IPC
/// 2. Do any required communication to set up the parent / child communication channels
/// 3. Return the communication channels for the child
///
/// HINT: You can change the signature of the function if necessasry
///
fn connect_to_coordinator(opts: &tpcoptions::TPCOptions) -> (Sender<ProtocolMessage>, Receiver<ProtocolMessage>) {
    let ipc = &opts.ipc_path.clone();
    let (server, serverName): (IpcOneShotServer<Sender<ProtocolMessage>>, String) = IpcOneShotServer::new().unwrap();
    let (tx0, rx0): (Sender<ProtocolMessage>, Receiver<ProtocolMessage>) = channel().unwrap();
    let temp: Sender<(String, Sender<ProtocolMessage>)> = Sender::connect(ipc.to_string()).unwrap();
    temp.send((serverName, tx0));
    // Checkpoint 1
    let (_, recievedVal) = server.accept().unwrap();
    (recievedVal, rx0)
}

///
/// pub fn run(opts: &tpcoptions:TPCOptions, running: Arc<AtomicBool>)
///     opts: An options structure containing the CLI arguments
///     running: An atomically reference counted (ARC) AtomicBool(ean) that is
///         set to be false whenever Ctrl+C is pressed
///
/// 1. Creates a new coordinator
/// 2. Spawns and connects to new clients processes and then registers them with
///    the coordinator
/// 3. Spawns and connects to new participant processes and then registers them
///    with the coordinator
/// 4. Starts the coordinator protocol
/// 5. Wait until the children finish execution
///
fn run(opts: & tpcoptions::TPCOptions, running: Arc<AtomicBool>) {
    let coord_log_path = format!("{}//{}", opts.log_path, "coordinator.log");

    // TODO
    // New coordinator
    let (clientTx, clientRx) = channel().unwrap();
    let (partTx, partRx) = channel().unwrap();
    let coordinator = &mut Coordinator::new(coord_log_path, &running, clientRx, partRx);

    // spawn new clients
    for i in 0..opts.num_clients {

        // Send it the transmitting end
        let child_opts = &mut tpcoptions::TPCOptions::new();
        child_opts.send_success_probability = opts.send_success_probability;
        child_opts.operation_success_probability = opts.operation_success_probability;
        child_opts.verbosity = opts.verbosity;
        child_opts.mode = "client".to_string();
        child_opts.log_path = opts.log_path.clone();
        child_opts.num = i;
        coordinator.client_join(&"temp".to_string(), spawn_child_and_connect(child_opts, &mut clientTx.clone()));
    }


    // spawn new clients
    for i in 0..opts.num_participants {
        // spawn child and connect function
        let child_opts = &mut tpcoptions::TPCOptions::new();
        child_opts.send_success_probability = opts.send_success_probability;
        child_opts.operation_success_probability = opts.operation_success_probability;
        child_opts.verbosity = opts.verbosity;
        child_opts.mode = "participant".to_string();
        child_opts.log_path = opts.log_path.clone();
        child_opts.num = i;
        coordinator.participant_join(&"temp".to_string(), spawn_child_and_connect(child_opts, &mut partTx.clone()));
    }
    // Start Coordinator Protocol
    coordinator.protocol(opts.num_clients * opts.num_requests);

}

///
/// pub fn run_client(opts: &tpcoptions:TPCOptions, running: Arc<AtomicBool>)
///     opts: An options structure containing the CLI arguments
///     running: An atomically reference counted (ARC) AtomicBool(ean) that is
///         set to be false whenever Ctrl+C is pressed
///
/// 1. Connects to the coordinator to get tx/rx
/// 2. Constructs a new client
/// 3. Starts the client protocol
///
fn run_client(opts: & tpcoptions::TPCOptions, running: Arc<AtomicBool>) {
    // TODO
    // let (tx, rx) = connect_to_coordinator(opts);
    // println!("Client Running\n");
    // tx.send(ProtocolMessage::generate(MessageType::ClientRequest, "HI".to_string(), "123".to_string(), 2));
    let client_id_str = format!("client_{}", opts.num);
    
    let (tx, rx) = connect_to_coordinator(opts);
    let client = &mut Client::new(client_id_str, opts.num, running,  rx, tx);
    client.protocol(opts.num_requests);
}

///
/// pub fn run_participant(opts: &tpcoptions:TPCOptions, running: Arc<AtomicBool>)
///     opts: An options structure containing the CLI arguments
///     running: An atomically reference counted (ARC) AtomicBool(ean) that is
///         set to be false whenever Ctrl+C is pressed
///
/// 1. Connects to the coordinator to get tx/rx
/// 2. Constructs a new participant
/// 3. Starts the participant protocol
///
fn run_participant(opts: & tpcoptions::TPCOptions, running: Arc<AtomicBool>) {
    let participant_id_str = format!("participant_{}", opts.num);
    let participant_log_path = format!("{}//{}.log", opts.log_path, participant_id_str);

    let (tx, rx) = connect_to_coordinator(opts);
    let participant = &mut Participant::new(participant_id_str, participant_log_path, running, opts.send_success_probability, opts.operation_success_probability, rx, tx);
    participant.protocol();
}

fn main() {
    // Parse CLI arguments
    let opts = tpcoptions::TPCOptions::new();
    // Set-up logging and create OpLog path if necessary
    stderrlog::new()
            .module(module_path!())
            .quiet(false)
            .timestamp(stderrlog::Timestamp::Millisecond)
            .verbosity(opts.verbosity)
            .init()
            .unwrap();
    match fs::create_dir_all(opts.log_path.clone()) {
        Err(e) => error!("Failed to create log_path: \"{:?}\". Error \"{:?}\"", opts.log_path, e),
        _ => (),
    }

    // Set-up Ctrl-C / SIGINT handler
    let running = Arc::new(AtomicBool::new(true));
    let r = running.clone();
    let m = opts.mode.clone();
    ctrlc::set_handler(move || {
        r.store(false, Ordering::SeqCst);
        if m == "run" {
            print!("\n");
        }
    }).expect("Error setting signal handler!");

    // Execute main logic
    match opts.mode.as_ref() {
        "run" => run(&opts, running),
        "client" => run_client(&opts, running),
        "participant" => run_participant(&opts, running),
        "check" => checker::check_last_run(opts.num_clients, opts.num_requests, opts.num_participants, &opts.log_path),
        _ => panic!("Unknown mode"),
    }
}
