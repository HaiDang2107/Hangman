# Quick Start Guide

## Compilation

```bash
cd /home/haidang/Desktop/Network\ Programming
make clean
make
```

**Output**: `server` executable

## Running the Server

```bash
./server 5000
```

Output:
```
Server listening on port 5000
```

Press `Ctrl+C` to shutdown gracefully.

## Project Status

### ✅ Completed Steps

1. **Socket and Listener** - Port binding and client acceptance
2. **Event Loop** - Epoll-based I/O multiplexing
3. **Connection Management** - Per-client buffers and packet handling
4. **Packet Protocol** - Complete serialization framework
5. **Task Queue** - Thread-safe work queue
6. **Callback Queue** - Worker-to-network communication via eventfd
7. **Server Integration** - Full orchestration of all components
8. **Concrete Tasks** - Login and Register implementations

### 📋 TODO: Service Layer Implementation

Create business logic services in `backend/service/`:

```cpp
// Example: AuthService.h
class AuthService {
public:
    S2C_LoginResult login(const std::string& user, const std::string& pass);
    S2C_RegisterResult registerUser(const std::string& user, const std::string& pass);
    bool validateToken(const std::string& token);
};

// Then integrate into tasks:
void LoginTask::execute() {
    result = AuthService::getInstance().login(...);
}
```

## File Organization

```
backend/
├── include/
│   ├── network/           (sockets, epoll, server)
│   ├── threading/         (tasks, queues, callbacks)
│   ├── protocol/          (packets, serialization)
│   └── service/           ⬅️ TO IMPLEMENT
├── src/
│   ├── network/
│   ├── threading/
│   ├── protocol/
│   └── service/           ⬅️ TO IMPLEMENT
└── database/
    ├── account.txt        (credentials)
    └── history/           (game records)
```

## Key Components

### Network Thread
- Runs EventLoop (epoll)
- Accepts client connections
- Reads/writes packets
- Processes callbacks

### Worker Thread
- Pops tasks from TaskQueue
- Executes blocking operations (DB access)
- Pushes callbacks to CallbackQueue
- Notifies network thread via eventfd

### Queue Communication
```
Network Thread          Worker Thread
    │                        │
    ├─ push(task) ──────────>│
    │                        │
    │                  execute(task)
    │                        │
    │<────── push(callback) ──
    │
    └─ process callback on EventFD event
```

## Adding New Packet Types

1. **Define packet** in `packets.h`
2. **Add serialization** in `packets.cpp` (to_bytes, from_payload)
3. **Create task** in `Task.h` (inherit from Task)
4. **Implement task** in `Task.cpp` (execute and onComplete)
5. **Route in server** in `Server::processPacket()`
6. **Implement service** for business logic

## Database

### Account File
Location: `backend/database/account.txt`
Format: `username:password\n`

### Create Test Account
```bash
echo "testuser:testpass" >> backend/database/account.txt
```

## Architecture Highlights

```
Single Network Thread     One Worker Thread        Database
        │                         │                   │
        │                         │                   │
    EventLoop              TaskQueue            account.txt
        │                    │                    │
        ├─ epoll             ├─ Thread-safe      └─ Credentials
        ├─ callbacks         ├─ Blocking pop       │
        └─ multiplexing      └─ Stop signal        └─ User data
        │                         │
    Connections            Tasks (Login, Register, etc)
        │                         │
        ├─ Per-client            ├─ execute() on worker
        ├─ Send/recv buffers     └─ onComplete() on network
        └─ Partial I/O
        │                         │
        └─────────────────────────┘
               CallbackQueue
                  │
                EventFD
                  │
              Notification
```

## Testing

### Test Login
```bash
# Terminal 1
./server 5000

# Terminal 2: Connect and send login packet
telnet localhost 5000
# (Would need binary packet format)
```

### Test Account Creation
```bash
echo "newuser:newpass" >> backend/database/account.txt
cat backend/database/account.txt
```

## Performance Notes

- **Connections**: Handles thousands with O(n) complexity
- **I/O**: Non-blocking, edge-triggered epoll
- **Threading**: One worker thread for tasks (upgrade to thread pool if needed)
- **Memory**: Per-connection buffers (8KB send, 8KB recv)

## Common Issues

### Build Errors
```bash
make clean  # Remove old .o files
make        # Fresh compile
```

### Port Already in Use
```bash
lsof -i :5000
kill -9 <PID>
./server 5000
```

### Database Permissions
```bash
touch backend/database/account.txt
chmod 666 backend/database/account.txt
```

## Next Steps

1. **Implement Service Layer**: AuthService, RoomService, GameService
2. **Add Packet Handlers**: Map packet types to tasks in processPacket()
3. **Implement Game Logic**: Word selection, guessing, scoring
4. **Add Persistence**: Room state, game history, leaderboard
5. **Enhance Error Handling**: Comprehensive exception handling
6. **Add Logging**: Debug and audit logs
7. **Security**: Token encryption, password hashing

## Code Statistics

- **Lines of Code**: ~3000
- **Headers**: 10
- **Source Files**: 10
- **Compilation Time**: <1 second
- **Binary Size**: ~200KB

## Support

Refer to these documents for more details:
- `IMPLEMENTATION.md` - Complete implementation overview
- `ARCHITECTURE.md` - System architecture and design
- `SERVICE_IMPLEMENTATION.md` - Service layer patterns

## Contact

For questions about the implementation, refer to the code comments and documentation files.
