#!/bin/bash
# ============================================================
#  setup.sh — One-shot project setup
#  Run this ONCE after unzipping.
#  Usage: chmod +x setup.sh && ./setup.sh
# ============================================================

set -e

echo ""
echo "╔══════════════════════════════════════════╗"
echo "║   GREEN THREAD OS SIMULATOR — SETUP      ║"
echo "╚══════════════════════════════════════════╝"
echo ""

# ── 1. Compile C++ ───────────────────────────────────────────
echo "🔨 [1/3] Compiling C++ simulator ..."
cd cpp
chmod +x build.sh
./build.sh
cd ..
echo ""

# ── 2. Install backend deps ───────────────────────────────────
echo "📦 [2/3] Installing backend dependencies ..."
cd backend
npm install
cd ..
echo ""

# ── 3. Install frontend deps ──────────────────────────────────
echo "📦 [3/3] Installing frontend dependencies ..."
cd frontend
npm install
cd ..
echo ""

echo "✅  Setup complete!"
echo ""
echo "Now start the app with:"
echo "  Terminal 1 → cd backend  && npm start"
echo "  Terminal 2 → cd frontend && npm run dev"
echo ""
echo "Then open: http://localhost:5173"
echo ""
