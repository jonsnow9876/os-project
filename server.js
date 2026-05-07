const express = require('express');
const cors = require('cors');

const app = express();
const PORT = 5000;

app.use(cors());
app.use(express.json());

// Default config
let simConfig = {
    threads: 3,
    iterations: 4
};

// ✅ Health check
app.get('/api/health', (req, res) => {
    res.json({ status: 'ok', config: simConfig });
});

// ✅ Config endpoint
app.post('/api/config', (req, res) => {
    const { threads, iterations } = req.body;

    simConfig.threads = Math.min(8, Math.max(1, threads || 3));
    simConfig.iterations = Math.min(10, Math.max(1, iterations || 4));

    res.json({ success: true, config: simConfig });
});

// 🔥 MAIN FIX: No C++ → Simulated execution
app.post('/api/run', (req, res) => {
    const threads = req.body.threads || simConfig.threads;
    const iterations = req.body.iterations || simConfig.iterations;

    let logs = [];

    logs.push({
        type: "SYSTEM",
        message: "Starting Scheduler...",
        timestamp: Date.now()
    });

    // Create threads
    for (let t = 1; t <= threads; t++) {
        logs.push({
            type: "SYSTEM",
            message: `Thread-${t} created`,
            threadId: t,
            state: "READY",
            timestamp: Date.now()
        });
    }

    // Simulate execution
    for (let i = 1; i <= iterations; i++) {
        for (let t = 1; t <= threads; t++) {

            logs.push({
                type: "THREAD",
                message: `Thread-${t} STATE:RUNNING (Iteration ${i})`,
                threadId: t,
                state: "RUNNING",
                timestamp: Date.now()
            });

            logs.push({
                type: "MUTEX",
                message: `Thread-${t} acquired lock`,
                threadId: t,
                state: "RUNNING",
                timestamp: Date.now()
            });

            logs.push({
                type: "MUTEX",
                message: `Thread-${t} released lock`,
                threadId: t,
                state: "READY",
                timestamp: Date.now()
            });
        }
    }

    logs.push({
        type: "SYSTEM",
        message: "All threads finished execution",
        timestamp: Date.now()
    });

    res.json({
        success: true,
        logs,
        config: { threads, iterations },
        totalLogs: logs.length
    });
});

// ✅ Start server
app.listen(PORT, () => {
    console.log(`🟢 Backend running on http://localhost:${PORT}`);
});