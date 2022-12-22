//!
//! coordinator.rs
//! Implementation of 2PC coordinator
//!
extern crate log;
extern crate stderrlog;
extern crate rand;
extern crate ipc_channel;

use std::collections::HashMap;
use std::sync::Arc;
use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};
use std::thread;
use std::time::Duration;
use std::process::{Child,Command};

use coordinator::ipc_channel::ipc::IpcSender as Sender;
use coordinator::ipc_channel::ipc::IpcReceiver as Receiver;
use coordinator::ipc_channel::ipc::TryRecvError;
use coordinator::ipc_channel::ipc::channel;

use message;
use message::MessageType;
use message::ProtocolMessage;
use message::RequestStatus;
use oplog;

use crate::participant;

/// CoordinatorState
/// States for 2PC state machine
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum CoordinatorState {
    Quiescent,
    ReceivedRequest,
    ProposalSent,
    ReceivedVotesAbort,
    ReceivedVotesCommit,
    SentGlobalDecision
}

/// Coordinator
/// Struct maintaining state for coordinator
#[derive(Debug)]
pub struct Coordinator {
    state: CoordinatorState,
    running: Arc<AtomicBool>,
    log: oplog::OpLog,
    clientsReceiver: Receiver<ProtocolMessage>,
    clients: Vec<(Child, Sender<ProtocolMessage>)>,
    participantsReceiver: Receiver<ProtocolMessage>,
    participants: Vec<(Child, Sender<ProtocolMessage>)>,
}

///
/// Coordinator
/// Implementation of coordinator functionality
/// Required:
/// 1. new -- Constructor
/// 2. protocol -- Implementation of coordinator side of protocol
/// 3. report_status -- Report of aggregate commit/abort/unknown stats on exit.
/// 4. participant_join -- What to do when a participant joins
/// 5. client_join -- What to do when a client joins
///
impl Coordinator {

    ///
    /// new()
    /// Initialize a new coordinator
    ///
    /// <params>
    ///     log_path: directory for log files --> create a new log there.
    ///     r: atomic bool --> still running?
    ///
    pub fn new(
        log_path: String,
        r: &Arc<AtomicBool>,
        clientRx: Receiver<ProtocolMessage>,
        participantRx: Receiver<ProtocolMessage>) -> Coordinator {

        Coordinator {
            state: CoordinatorState::Quiescent,
            log: oplog::OpLog::new(log_path),
            running: r.clone(),
            clientsReceiver: clientRx,
            clients: Vec::new(),
            participantsReceiver: participantRx,
            participants: Vec::new(),   
        }
    }

    ///
    /// participant_join()
    /// Adds a new participant for the coordinator to keep track of
    ///
    /// HINT: Keep track of any channels involved!
    /// HINT: You may need to change the signature of this function
    ///
    pub fn participant_join(&mut self, name: &String, participant: (Child, Sender<ProtocolMessage>)) {
        assert!(self.state == CoordinatorState::Quiescent);
        self.participants.push(participant);
        // TODO
    }

    ///  
    /// client_join()
    /// Adds a new client for the coordinator to keep track of
    ///
    /// HINT: Keep track of any channels involved!
    /// HINT: You may need to change the signature of this function
    ///
    pub fn client_join(&mut self, name: &String, client: (Child, Sender<ProtocolMessage>)) {
        assert!(self.state == CoordinatorState::Quiescent);
        self.clients.push(client);
        // TODO
    }

    // pub fn recieve(&mut self) {
    //     let temp = self.clientsReceiver.recv();
    //     print!("got this from client: {}\n", temp.unwrap().senderid);
    // }

    // pub fn send(&mut self, num: u32) {
    //     print!("sending this to client: {}\n", num);
    //     let sendingString = format!("{}", num);
    //     self.clients[num as usize].1.send(ProtocolMessage::generate(MessageType::ClientRequest, "HI".to_string(), sendingString.to_string(), 2));
    // }

    ///
    /// report_status()
    /// Report the abort/commit/unknown status (aggregate) of all transaction
    /// requests made by this coordinator before exiting.
    ///
    pub fn report_status(&mut self, successful_ops: u64, failed_ops: u64) {
        // TODO: Collect actual stats
        let unknown_ops: u64 = 0;

        println!("coordinator     :\tCommitted: {:6}\tAborted: {:6}\tUnknown: {:6}", successful_ops, failed_ops, unknown_ops);
    }

    ///
    /// protocol()
    /// Implements the coordinator side of the 2PC protocol
    /// HINT: If the simulation ends early, don't keep handling requests!
    /// HINT: Wait for some kind of exit signal before returning from the protocol!
    ///
    pub fn protocol(&mut self, numTasks: u32 ) {
        let mut successes = 0;
        let mut fails = 0;

        // TODO
        for i in 0..numTasks {
            // Wait for work from the client
            if(!self.running.load(Ordering::Relaxed)){
                for client in self.clients.iter() {
                    client.1.send(ProtocolMessage::instantiate(MessageType::CoordinatorExit, 1, "".to_string(), "".to_string(), 1));
                }
                break;
            }
            let task = self.clientsReceiver.recv().unwrap();
            let clientNum: i32 = task.senderid.parse().unwrap();
            let uid = task.uid;
            let tid = task.txid;
            let oid = task.opid;

            // Query to commit
            for (index, (_ , participant)) in self.participants.iter().enumerate() {
                let message = ProtocolMessage::instantiate(MessageType::CoordinatorPropose, uid, tid.to_string(), index.to_string(), oid);
                participant.send(message);
                self.log.append(MessageType::CoordinatorPropose, tid.to_string(), index.to_string(), oid);
            }

            // await votes
            let mut commit = true;
            let mut numVotes = 0;
            thread::sleep(Duration::from_millis(50));
            loop{ 
                let check = self.participantsReceiver.try_recv();
                if !(check.is_ok()) {
                    break;
                }
                let temp = check.unwrap();
                numVotes += 1;
                if (temp.mtype == MessageType::ParticipantVoteAbort) {
                    commit = false;
                }
            }

            if (numVotes < self.participants.len()){
                commit = false;
            }

            // Either commit or abort
            let mut res = MessageType::CoordinatorCommit;
            let mut clientRes = MessageType::ClientResultCommit;
            if (!commit) {
                // record stats here...
                clientRes = MessageType::ClientResultAbort;
                res = MessageType::CoordinatorAbort;
                fails += 1;
            } else {
                successes += 1;
            }

            // tell participants result
            for (index, (_, participant)) in self.participants.iter().enumerate() {
                let message = ProtocolMessage::instantiate(res, uid, tid.to_string(), index.to_string(), oid);
                participant.send(message);
            }
            self.log.append(res, tid.to_string(), "".to_string(), oid);
            
            // send results to client.
            let clientMsg = ProtocolMessage::instantiate(clientRes, uid, tid.to_string(), clientNum.to_string(), oid);
            self.log.append(clientRes, tid.to_string(), clientNum.to_string(), oid);
            self.clients[clientNum as usize].1.send(clientMsg);
        }

        for participant in self.participants.iter() {
            // TODO: Add something for if a participant doesn't respond to the request to vote. (in which case, we abort)
            participant.1.send(ProtocolMessage::instantiate(MessageType::CoordinatorExit, 0, "".to_string(), "".to_string(), 0));
        }

        while self.running.load(Ordering::Relaxed){
            thread::sleep(Duration::from_millis(100));
        }

        self.report_status(successes, fails);
    }
}
