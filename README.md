# Green Thread OS Simulator 🧵

A full-stack visualization dashboard for a **user-level (green) thread scheduler** written in C++, with a Node.js/Express backend and a React frontend.

Demonstrates: **context switching**, **round-robin scheduling**, and **mutex locking** — all the core OS concepts you need for your viva.

---

## 📁 Folder Structure

```
green-thread-os/
├── cpp/
│   ├── green_thread.cpp   ← C++ simulator (ucontext, round-robin, mutex)
│   └── build.sh           ← Compiles the C++ binary
│
├── backend/
│   ├── server.js          ← Express API (auto-compiles & runs the C++ binary)
│   └── package.json
│
├── frontend/
│   ├── index.html
│   ├── vite.config.js
│   ├── package.json
│   └── src/
│       ├── main.jsx
│       ├── App.jsx
│       ├── index.css
│       └── components/
│           ├── Header.jsx           ← Top bar
│           ├── ControlPanel.jsx     ← Thread/iter sliders + Run button
│           ├── ThreadVisualizer.jsx ← Animated state cards (READY/RUNNING/BLOCKED)
│           ├── Timeline.jsx         ← Gantt-style execution timeline
│           ├── LogConsole.jsx       ← Color-coded live logs
│           └── ConceptCards.jsx     ← OS concept explainer (viva helper)
│
├── setup.sh     ← One-shot installer
└── README.md
```

---

## ⚡ Quick Start (2 Commands)

### Prerequisites

| Tool    | Version  | Check                |
|---------|----------|----------------------|
| Node.js | ≥ 18     | `node --version`     |
| g++     | ≥ 9      | `g++ --version`      |
| npm     | ≥ 9      | `npm --version`      |

> On Ubuntu/WSL: `sudo apt install g++ nodejs npm`  
> On macOS: `brew install gcc node`

---

### Step 1 — Setup (run once)

```bash
chmod +x setup.sh
./setup.sh
```

This will:
- Compile `green_thread.cpp` → `cpp/green_thread` binary
- Run `npm install` in `backend/`
- Run `npm install` in `frontend/`

---

### Step 2 — Start the app

**Terminal 1 (backend):**
```bash
cd backend
npm start
# Backend running at http://localhost:5000
```

**Terminal 2 (frontend):**
```bash
cd frontend
npm run dev
# Frontend running at http://localhost:5173
```

**Open your browser:** [http://localhost:5173](http://localhost:5173)

---

## 🖥️ Manual Steps (if setup.sh fails)

```bash
# 1. Compile C++
cd cpp
g++ -std=c++17 -O2 -Wall -o green_thread green_thread.cpp
cd ..

# 2. Backend
cd backend
npm install
node server.js &
cd ..

# 3. Frontend
cd frontend
npm install
npm run dev
```

---

## 🔌 API Reference

| Method | Endpoint     | Body                            | Response                       |
|--------|--------------|---------------------------------|--------------------------------|
| GET    | /api/health  | —                               | `{ status: "ok" }`             |
| POST   | /api/config  | `{ threads, iterations }`       | `{ success, config }`          |
| POST   | /api/run     | `{ threads?, iterations? }`     | `{ success, logs[], config }`  |

### Example

```bash
curl -X POST http://localhost:5000/api/run \
  -H "Content-Type: application/json" \
  -d '{"threads": 3, "iterations": 4}'
```

---

## 🧠 Core OS Concepts (Viva Prep)

### 1. Green Threads
- User-space threads managed by a **runtime library**, not the OS kernel
- Faster to create and switch than OS threads (no syscall overhead)
- All run on one OS thread → no true parallelism, but great for I/O-bound work

### 2. Context Switching (`ucontext`)
```
getcontext(&ctx)      → Snapshot current CPU state into ctx
makecontext(&ctx, fn) → Set function to execute when ctx is activated
swapcontext(from, to) → Save *from, load *to, jump
```

### 3. Round-Robin Scheduling
```
Ready Queue: [T0, T1, T2]
Cycle 1: T0 runs → yields → [T1, T2, T0]
Cycle 2: T1 runs → yields → [T2, T0, T1]
...
```
- No thread starves
- Every thread gets equal CPU time per cycle

### 4. Mutex
```
Lock free?  → acquire, enter critical section
Lock held?  → BLOCKED state, added to wait queue
Unlock      → wake next waiter → READY state
```

---

## 🎨 Dashboard Features

| Feature                | Description                                       |
|------------------------|---------------------------------------------------|
| Thread State Cards     | Real-time READY/RUNNING/BLOCKED/FINISHED display  |
| Execution Timeline     | Gantt-style slot-per-context-switch visualization |
| Color-coded Log Console| GREEN=thread events, CYAN=switches, AMBER=mutex   |
| Concept Cards          | Collapsible OS concept explanations               |
| Config Panel           | Sliders for 1–8 threads, 1–10 iterations          |

---

## 🔧 Troubleshooting

**"Cannot reach backend"**  
→ Make sure `cd backend && npm start` is running

**"Compilation failed"**  
→ Check `g++` is installed: `g++ --version`  
→ On macOS, install Xcode Command Line Tools: `xcode-select --install`

**Port conflict**  
→ Backend port: edit `PORT` in `backend/server.js`  
→ Frontend port: edit `port` in `frontend/vite.config.js`

**ucontext deprecated warning on macOS**  
→ Add `-Wno-deprecated` to the g++ flags in `cpp/build.sh` — still works fine

---

## 📜 License

MIT — free for academic and educational use.
