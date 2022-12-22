//!
//! client.rs
//! Implementation of 2PC client
//!
extern crate ipc_channel;
extern crate log;
extern crate stderrlog;

use std::thread;
use std::time::Duration;
use std::sync::Arc;
use std::sync::atomic::{AtomicBool, Ordering};
use std::collections::HashMap;

use client::ipc_channel::ipc::IpcReceiver as Receiver;
use client::ipc_channel::ipc::TryRecvError;
use client::ipc_channel::ipc::IpcSender as Sender;

use message;
use message::ProtocolMessage;
use message::MessageType;
use message::RequestStatus;

// Client state and primitives for communicating with the coordinator
#[derive(Debug)]
pub struct Client {
    pub id_str: String,
    num: u32,
    pub running: Arc<AtomicBool>,
    pub num_requests: u32,
    pub rx: Receiver<ProtocolMessage>,
    pub tx: Sender<ProtocolMessage>
}

///
/// Client Implementation
/// Required:
/// 1. new -- constructor
/// 2. pub fn report_status -- Reports number of committed/aborted/unknown
/// 3. pub fn protocol(&mut self, n_requests: i32) -- Implements client side protocol
///
impl Client {

    ///
    /// new()
    ///
    /// Constructs and returns a new client, ready to run the 2PC protocol
    /// with the coordinator.
    ///
    /// HINT: You may want to pass some channels or other communication
    ///       objects that enable coordinator->client and client->coordinator
    ///       messaging to this constructor.
    /// HINT: You may want to pass some global flags that indicate whether
    ///       the protocol is still running to this constructor
    ///
    pub fn new(id_str: String,
               num: u32,
               running: Arc<AtomicBool>,
               rx: Receiver<ProtocolMessage>,
               tx: Sender<ProtocolMessage>) -> Client {
        Client {
            id_str: id_str,
            num: num,
            running: running,
            num_requests: 0,
            rx: rx,
            tx: tx
            // TODO
        }
    }

    ///
    /// wait_for_exit_signal(&mut self)
    /// Wait until the running flag is set by the CTRL-C handler
    ///
    pub fn wait_for_exit_signal(&mut self) {
        trace!("{}::Waiting for exit signal", self.id_str.clone());

        while self.running.load(Ordering::Relaxed){
            thread::sleep(Duration::from_millis(100));
        }

        trace!("{}::Exiting", self.id_str.clone());
    }

    ///
    /// send_next_operation(&mut self)
    /// Send the next operation to the coordinator
    ///
    pub fn send_next_operation(&mut self) {

        // Create a new request with a unique TXID.
        let txid = format!("{}_op_{}", self.id_str.clone(), self.num_requests);
        let pm = message::ProtocolMessage::generate(message::MessageType::ClientRequest,
                                                    txid.clone(),
                                                    self.num.to_string(),
                                                    self.num_requests);
        info!("{}::Sending operation #{}", self.id_str.clone(), self.num_requests);

        // TODO
        self.tx.send(pm);

        trace!("{}::Sent operation #{}", self.id_str.clone(), self.num_requests);
    }

    ///
    /// recv_result()
    /// Wait for the coordinator to respond with the result for the
    /// last issued request. Note that we assume the coordinator does
    /// not fail in this simulation
    ///
    pub fn recv_result(&mut self) -> i32 {

        // TODO
        let result = self.rx.recv().unwrap();
        match result.mtype {
            (MessageType::ClientResultCommit) => {1}
            (MessageType::ClientResultAbort) => {0}
            _ => {-1}
        }
    }

    ///
    /// report_status()
    /// Report the abort/commit/unknown status (aggregate) of all transaction
    /// requests made by this client before exiting.
    ///
    pub fn report_status(&mut self, successful_ops: u64, failed_ops: u64) {
        // TODO: Collect actual stats
        // let successful_ops: u64 = 0;
        // let failed_ops: u64 = 0;
        let unknown_ops: u64 = 0;

        println!("{:16}:\tCommitted: {:6}\tAborted: {:6}\tUnknown: {:6}", self.id_str.clone(), successful_ops, failed_ops, unknown_ops);
    }

    ///
    /// protocol()
    /// Implements the client side of the 2PC protocol
    /// HINT: if the simulation ends early, don't keep issuing requests!
    /// HINT: if you've issued all your requests, wait for some kind of
    ///       exit signal before returning from the protocol method!
    ///
    pub fn protocol(&mut self, n_requests: u32) {
        let mut successes = 0;
        let mut fails = 0;
        // TODO
        for i in 0..n_requests{
            // send and receive. 
            // Send task to client
            self.send_next_operation();
            self.num_requests += 1;
            let successful = self.recv_result();
            if (successful == 1) {
                successes += 1;
            }else if (successful == 0) {
                fails += 1;
            }else {
                break;
            }
        }
        // println!("client sent {} requests {}", self.num_requests, n_requests);
        // self.running;
        self.wait_for_exit_signal();
        self.report_status(successes, fails);
    }
}
