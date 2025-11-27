# Network Server Architecture

## High-Level Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        HANGMAN SERVER                           │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│  NETWORK THREAD (Main)                                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────┐        ┌──────────────────────────┐   │
│  │   EventLoop          │        │  Server Orchestrator     │   │
│  │  (Epoll)             │        │                          │   │
│  │                      │        │  - Accept connections    │   │
│  │  - File descriptors  │        │  - Route packets         │   │
│  │  - Callbacks         │        │  - Send responses        │   │
│  │  - Edge-triggered    │        │  - Process callbacks     │   │
│  └──────────────────────┘        └──────────────────────────┘   │
│         │                                        │              │
│         └─────────────┬──────────────────────────┘              │
│                       │                                         │
│     ┌─────────────────┴──────────────────┐                      │
│     │                                    │                      │
│  ┌──────────────┐          ┌─────────────────────┐              │
│  │ Connections  │          │ CallbackQueue       │              │
│  │              │          │ (EventFD notify)    │              │
│  │ - Send Buf   │          │                     │              │
│  │ - Recv Buf   │          │ - Worker callbacks  │              │
│  │ - Per-client │          │ - Non-blocking      │              │
│  └──────────────┘          └─────────────────────┘              │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
         │                          │
         │                          │
    [Packets]                 [Callbacks]
         │                          │
         │                          │
         ▼                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  WORKER THREAD (Background)                                     │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │        TaskQueue (Thread-Safe)                           │   │
│  │                                                          │   │
│  │  - Task 1                                                │   │
│  │  - Task 2                                                │   │
│  │  - ...                                                   │   │
│  └──────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │     Task Execution (Blocking Operations OK)              │   │
│  │                                                          │   │
│  │  ┌────────────────┐    ┌────────────────┐                │   │
│  │  │ LoginTask      │    │ RegisterTask   │                │   │
│  │  │ - DB access    │    │ - DB access    │                │   │
│  │  │ - Validation   │    │ - Validation   │                │   │
│  │  │ - Token gen    │    │ - File I/O     │                │   │
│  │  └────────────────┘    └────────────────┘                │   │
│  │                                                          │   │
│  │  ┌────────────────┐    ┌────────────────┐                │   │
│  │  │ RoomTask       │    │ GameTask       │                │   │
│  │  │ - Room logic   │    │ - Game logic   │                │   │
│  │  │ - Persistence  │    │ - Score calc   │                │   │
│  │  └────────────────┘    └────────────────┘                │   │
│  │                                                          │   │
│  └──────────────────────────────────────────────────────────┘   │
│         │                                                       │
│         ▼                                                       │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Callback Generation                                      │   │
│  │                                                          │   │
│  │ Push result to CallbackQueue with status                 │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────┐
│ PROTOCOL LAYER                                                  │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  Packet Format:                                                 │
│  ┌──────────────────────┐ ┌──────────────────────────────────┐  │
│  │      HEADER (7B)     │ │    PAYLOAD (variable)            │  │
│  ├──┬────┬──────────────┤ │                                  │  │
│  │V │Type│ PayloadLen   │ │  Serialized packet data          │  │
│  │1B│ 2B │     4B       │ │  (strings, ints, etc)            │  │
│  └──┴────┴──────────────┘ └──────────────────────────────────┘  │
│                                                                 │
│  Packet Types:                                                  │
│  - C2S_Login (0x0103)      S2C_LoginResult (0x0104)             │
│  - C2S_Register (0x0101)   S2C_RegisterResult (0x0102)          │
│  - C2S_CreateRoom (0x0201) S2C_CreateRoomResult (0x0202)        │
│  - C2S_GuessChar (0x0501)  S2C_GuessCharResult (0x0502)         │
│  - ... and more                                                 │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────┐
│ DATA PERSISTENCE LAYER                                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  database/account.txt  - User credentials                       │
│  database/history/     - Game history per user                  │
│  database/sessions/    - Active session tokens (optional)       │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

## Communication Flow

### Login Sequence

```
CLIENT                   NETWORK THREAD              WORKER THREAD
  │                            │                            │
  ├─ C2S_Login ─────────────────>                           │
  │                            │                            │
  │                       Read packet                       │
  │                            │                            │
  │                       Create LoginTask                  │
  │                            │                            │
  │                       Queue to TaskQueue ───────────────>
  │                            │                            │
  │                            │                  Execute task
  │                            │                  - Read DB
  │                            │                  - Verify pwd
  │                            │                  - Gen token
  │                            │                  Create callback
  │                            <─── Push to CallbackQueue ──
  │                            │                            │
  │               Process callback on EventFD event         │
  │               Send S2C_LoginResult                      │
  │                            │                            │
  │  <─────── S2C_LoginResult ──                            │
  │                            │                            │
```

### Packet Processing

```
1. Client sends packet
   │
   ├─> Socket receives bytes
   │   └─> Connection.receiveData() buffers it
   │       └─> EventLoop detects read event
   │           └─> Server.handleClientRead()
   │
2. Server checks for complete packet
   │
   ├─> Connection.hasCompletePacket() checks header
   │   └─> Parse: version, type, length
   │
3. Route to handler
   │
   ├─> Server.processPacket() routes by type
   │   └─> Create appropriate Task
   │       └─> TaskQueue.push(task)
   │
4. Worker processes task
   │
   ├─> TaskQueue.pop() blocks until available
   │   └─> task->execute() runs on worker thread
   │       └─> Access DB, compute results
   │           └─> Store results in task
   │
5. Notify network thread
   │
   ├─> CallbackQueue.push(callback)
   │   └─> Write to eventfd (wakes epoll)
   │       └─> Server.handleCallbacks() triggered
   │
6. Send response
   │
   ├─> callback->execute() on network thread
   │   └─> task->onComplete() accesses result
   │       └─> Serialize response packet
   │           └─> Connection.sendData() queues it
   │               └─> EventLoop detects write ready
   │                   └─> Server.handleClientWrite()
   │                       └─> Write to socket
   │
7. Client receives response
   │
   └─> Network delivers S2C packet
```

## Thread Safety

```
SHARED RESOURCES:

┌─────────────────────────────────────────────┐
│  TaskQueue                                  │
│  ├─ Protected by: std::mutex                │
│  ├─ Accessed by: Network thread (push)      │
│  └─ Accessed by: Worker thread (pop)        │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│  CallbackQueue                              │
│  ├─ Protected by: std::mutex                │
│  ├─ Accessed by: Worker thread (push)       │
│  └─ Accessed by: Network thread (popAll)    │
└─────────────────────────────────────────────┘

┌─────────────────────────────────────────────┐
│  EventLoop (running flag)                   │
│  ├─ Protected by: std::atomic<bool>         │
│  └─ Accessed by: Both threads               │
└─────────────────────────────────────────────┘

THREAD-LOCAL RESOURCES:

┌─────────────────────────────────────────────┐
│  Connections map                            │
│  ├─ Protected by: None (network thread only)│
│  └─ Accessed by: Network thread             │
└─────────────────────────────────────────────┘

┌───────────────────────────────────────────────────┐
│  Individual Connection objects                    │
│  ├─ Protected by: None (per-client, immutable fd) │
│  └─ Accessed by: Network thread                   │
└───────────────────────────────────────────────────┘
```

## Scalability Characteristics

```
CLIENTS: O(n) connections handled with epoll
- No per-client thread
- No polling overhead
- Constant memory per connection

TASKS: Variable based on workload
- Worker thread processes sequentially
- Can add thread pool later for parallel execution

I/O: Non-blocking, multiplexed
- Edge-triggered for efficiency
- Buffers handle partial reads/writes
- No thread-per-connection overhead
```
